/* The geometry that is formed by the vertices from the vertex shader fills an area on the screen with fragments. The
 * fragment shader is invoked on these fragments to produce a color and depth for the frame buffer (or frame buffers)
*/
#version 450
/* This extension adds a "nonuniform" type qualifier and constructor, which is required by the Vulkan API to be used when
 * indexing descriptor bindings with an index that is not dynamically uniform. This extension also allows arrays of
 * resources declared using unsized arrays to become run-time sized arrays
 *
 * The 'require' behavior causes the named extension to work, if the implementation does not support the extension,
 * it fails
*/
#extension GL_EXT_nonuniform_qualifier: require
/* Note that the input variable does not necessarily have to use the same name, they will be linked together using the
 * indexes specified by the location directives
*/
layout (location = 0) in vec4 fragPosition;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) in vec4 fragNormal;
/* In the general case, there is not a 1:1 mapping between a vertex and a fragment. By default, the associated data per
 * vertex is interpolated across the primitive to generate the corresponding associated data per fragment. Using the flat
 * keyword, no interpolation is done, so every fragment generated during the rasterization of that particular primitive
 * will get the same data. Since primitives are usually defined by more than one vertex, this means that the data from
 * only one vertex is used in that case (this is called the provoking vertex)
*/
layout (location = 3) flat in uint fragDiffuseTexId;
layout (location = 4) flat in uint fragSpecularTexId;
layout (location = 5) flat in uint fragEmissionTexId;
layout (location = 6) flat in uint fragShininess;
/* Colors in GLSL are 4-component vectors with the R, G, B and alpha channels within the [0, 1] range. Unlike
 * gl_Position in the vertex shader, there is no built-in variable to output a color for the current fragment. You
 * have to specify your own output variable for each frame buffer where the layout modifier specifies the index of
 * the frame buffer. The color is written to the variable that is linked to the frame buffer at index specified in
 * location below
*/
layout (location = 0) out vec4 outColor;

struct LightInstanceDataSSBO {
    vec3 position;
    vec3 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    float constant;
    float linear;
    float quadratic;
    float innerRadiusCosine;
    float outerRadiusCosine;
};

layout (set = 0, binding = 1) readonly buffer LightInstanceData {
    LightInstanceDataSSBO instances[];
} lightInstanceData;

/* A combined image sampler descriptor is represented in GLSL by a sampler* uniform (where * is the type of a texture,
 * such a 1D, 2D, Cube, etc.). Whereas, the GLSL type sampler (no *) represents a descriptor of the form
 * VK_DESCRIPTOR_TYPE_SAMPLER
 *
 * Run time sized array
 * An array whose size is specified in its declaration or determined by its initializer is 'explicitly-sized'. An array
 * whose size is not specified in a declaration is 'unsized'. Unsized arrays can either be implicitly sized or
 * run-time sized. A 'run-time sized' array has its size determined by a buffer or descriptor set bound via the API
 *
 * Note that, only the final binding in a descriptor set can have a variable size
*/
layout (set = 1, binding = 0) uniform sampler2D texSampler[];

layout (push_constant) uniform SceneDataFragPC {
    layout(offset = 128)
    vec3 viewPosition;

    uint directionalLightsCount;
    uint pointLightsCount;
    uint spotLightsCount;
} sceneDataFrag;

vec3 addLight (LightInstanceDataSSBO light, vec4 lightDirection) {
    /* |------------------------------------------------------------------------------------------------------------|
     * | AMBIENT                                                                                                    |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* Light usually does not come from a single light source, but from many light sources scattered all around us,
     * even when they're not immediately visible. One of the properties of light is that it can scatter and bounce in
     * many directions, reaching spots that aren't directly visible; light can thus reflect on other surfaces and have
     * an indirect impact on the lighting of an object. Algorithms that take this into consideration are called global
     * illumination algorithms, but these are complicated and expensive to calculate. Ambient lighting is a very
     * simplistic model of global illumination
    */
    vec3 ambient = vec3 (light.ambient) *
                   vec3 (texture (texSampler[fragDiffuseTexId],  fragTexCoord));
    /* |------------------------------------------------------------------------------------------------------------|
     * | DIFFUSE                                                                                                    |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* Ambient lighting by itself doesn't produce the most interesting results, but diffuse lighting however will start
     * to give a significant visual impact on the object. Diffuse lighting gives the object more brightness the closer
     * its fragments are aligned to the light rays from a light source
     *
     * We need to measure at what angle the light ray touches the fragment. If the light ray is perpendicular to the
     * object's surface, the light has the greatest impact. To measure the angle between the light ray and the fragment
     * we use the normal vector, which is a vector perpendicular to the fragment's surface. The angle between the two
     * vectors can then easily be calculated with the dot product
     *
     * Note that, to get (only) the cosine of the angle between both vectors we will work with unit vectors (vectors of
     * length 1) so we need to make sure all the vectors are normalized, otherwise the dot product returns more than just
     * the cosine
     *
     * The resulting dot product thus returns a scalar that we can use to calculate the light's impact on the fragment's
     * color, resulting in differently lit fragments based on their orientation towards the light
     *
     * Note that, if the angle between both vectors is greater than 90 degrees then the result of the dot product will
     * actually become negative and we end up with a negative diffuse component. For that reason we use the max function
     * that returns the highest of both its parameters to make sure the diffuse component (and thus the colors) never
     * become negative
    */
    float diffuseIntensity = max  (dot (fragNormal, lightDirection), 0.0);
    vec3 diffuse           = vec3 (light.diffuse) * diffuseIntensity *
                             vec3 (texture (texSampler[fragDiffuseTexId],  fragTexCoord));
    /* |------------------------------------------------------------------------------------------------------------|
     * | SPECULAR                                                                                                   |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* Similar to diffuse lighting, specular lighting is based on the light's direction and the object's normal vectors,
     * but this time it is also based on the view direction e.g. from what direction the player is looking at the
     * fragment. Specular lighting is based on the reflective properties of surfaces. If we think of the object's
     * surface as a mirror, the specular lighting is the strongest wherever we would see the light reflected on the
     * surface
     *
     * We calculate a reflection vector by reflecting the light direction around the normal vector. Then we calculate
     * the angular distance between this reflection vector and the view direction. The closer the angle between them,
     * the greater the impact of the specular light. The resulting effect is that we see a bit of a highlight when we're
     * looking at the light's direction reflected via the surface. However, we will instead be using the Blinn-Phong
     * lighting model which doesn't use the reflection vector
     *
     * vec4 reflection = reflect (-lightDirection, fragNormal);
     *
     * The view direction can be calculated using the viewer's world space position (position vector of the camera
     * object) and the fragment's position. Then we calculate the specular's intensity, multiply this with the light
     * color and add this to the other components
     *
     * Note that, we chose to do the lighting calculations in world space, but most people tend to prefer doing lighting
     * in view space. An advantage of view space is that the viewer's position is always at (0,0,0) so you already got
     * the position of the viewer for free. However, calculating lighting in world space is more intuitive. If you want
     * to calculate lighting in view space you need to transform all the relevant vectors with the view matrix as well
     * (don't forget to change the normal matrix too)
    */
    vec4 viewDirection = normalize (vec4 (sceneDataFrag.viewPosition, 1.0) - fragPosition);
    /* With Phong lighting, we first calculate the dot product between the view direction and reflection vector (and
     * make sure it's not negative) and then raise it to the power of shininess value of the highlight. The higher the
     * shininess value of an object, the more it properly reflects the light instead of scattering it all around and
     * thus the smaller the highlight becomes
     *
     * Note that, Phong lighting is a great and very efficient approximation of lighting, but its specular reflections
     * break down when the angle between the view direciton and reflection vector goes over 90 degrees and the resulting
     * dot product becomes negative
     *
     * The Blinn-Phong shading model is an extension to the Phong shading model, it is largely similar, but approaches
     * the specular model slightly different which as a result overcomes our problem. Instead of relying on a reflection
     * vector we're using a so called halfway direction that is a unit vector exactly halfway between the view
     * direction and the light direction. The closer this halfway direction aligns with the surface's normal vector, the
     * higher the specular contribution
     *
     * Now, whatever direction the viewer looks from, the angle between the halfway direction and the surface's normal
     * vector never exceeds 90 degrees (unless the light is far below the surface of course). The results are slightly
     * different from Phong reflections, but generally more visually plausible, especially with low specular exponents
    */
    vec4 halfwayDirection   = normalize (viewDirection + lightDirection);
    float specularIntensity = pow  (max (dot (fragNormal, halfwayDirection), 0.0), fragShininess);
    vec3 specular           = vec3 (light.specular) * specularIntensity *
                              vec3 (texture (texSampler[fragSpecularTexId], fragTexCoord));
    /* Another subtle difference between Phong and Blinn-Phong shading is that the angle between the halfway direction
     * and the surface's normal vector is often shorter than the angle between the view direction and reflection vector.
     * As a result, to get visuals similar to Phong shading the specular shininess exponent has to be set a bit higher.
     * A general rule of thumb is to set it between 2 and 4 times the Phong shininess exponent
    */
    /* |------------------------------------------------------------------------------------------------------------|
     * | ATTENUATION                                                                                                |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* To reduce the intensity of light over the distance a light ray travels is generally called attenuation. One way
     * to reduce the light intensity over distance is to simply use a linear equation. Such an equation would linearly
     * reduce the light intensity over the distance thus making sure that objects at a distance are less bright. However,
     * such a linear function tends to look a bit fake. In the real world, lights are generally quite bright standing
     * close by, but the brightness of a light source diminishes quickly at a distance; the remaining light intensity
     * then slowly diminishes over distance
     *
     * The following formula calculates an attenuation value based on a fragment's distance to the light source
     *
     *                                  Fatt = 1.0 / (Kc + Kl * d + Kq * d^2)
     *
     * Here d represents the distance from the fragment to the light source. Then to calculate the attenuation value we
     * define 3 (configurable) terms:
     *
     * (1) A constant term Kc
     * The constant term is usually kept at 1.0 which is mainly there to make sure the denominator never gets smaller
     * than 1 since it would otherwise boost the intensity with certain distances, which is not the effect we're looking
     * for
     *
     * (2) A linear term Kl
     * The linear term is multiplied with the distance value that reduces the intensity in a linear fashion
     *
     * (3) A quadratic term Kq
     * The quadratic term is multiplied with the quadrant of the distance and sets a quadratic decrease of intensity for
     * the light source. The quadratic term will be less significant compared to the linear term when the distance is
     * small, but gets much larger as the distance grows
     *
     * Due to the quadratic term the light will diminish mostly at a linear fashion until the distance becomes large
     * enough for the quadratic term to surpass the linear term and then the light intensity will decrease a lot faster.
     * The resulting effect is that the light is quite intense when at a close range, but quickly loses its brightness
     * over distance until it eventually loses its brightness at a more slower pace
    */
    float distance    = length (vec4 (light.position, 1.0) - fragPosition);
    float attenuation = 1.0 /  (light.constant  +
                                light.linear    * distance +
                                light.quadratic * (distance * distance));
    /* Note that, we could leave the ambient component alone so ambient lighting is not decreased over distance, but
     * if we were to use more than one light source, all the ambient components will start to stack up. In that case we
     * want to attenuate ambient lighting as well
    */
    ambient          *= attenuation;
    diffuse          *= attenuation;
    specular         *= attenuation;

    return (ambient + diffuse + specular);
}
/* The main function is called for every fragment just like the vertex shader main function is called for every vertex
*/
void main (void) {
    /* When using multiple lights in a scene the approach is usually as follows: we have a single color vector that
     * represents the fragment's output color. For each light, the light's contribution to the fragment is added to this
     * output color vector. So each light in the scene will calculate its individual impact and contribute that to the
     * final output color
    */
    /* |------------------------------------------------------------------------------------------------------------|
     * | EMISSION                                                                                                   |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* The texture coordinate values will be smoothly interpolated across the area of the geometry by the rasterizer. We
     * can visualize this by having the fragment shader output the texture coordinates as colors. Note that, visualizing
     * data using colors is the shader programming equivalent of printf debugging, for lack of a better option. For
     * example, outColor = vec4 (fragTexCoord, 0.0, 1.0);
     *
     * Textures are sampled using the built-in texture function. It takes a sampler and coordinate as arguments. The
     * sampler automatically takes care of the filtering and transformations in the background
    */
    vec3 fragColor = vec3 (texture (texSampler[fragEmissionTexId], fragTexCoord));
    /* |------------------------------------------------------------------------------------------------------------|
     * | DIRECTIONAL LIGHTS                                                                                         |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* When a light source is far away, the light rays coming from the light source are close to parallel to each other.
     * It looks like all the light rays are coming from the same direction, regardless of where the object and/or the
     * viewer is. When a light source is modeled to be infinitely far away it is called a directional light since all its
     * light rays have the same direction; it is independent of the location of the light source, for example, the sun is
     * not infinitely far away from us, but it is so far away that we can perceive it as being infinitely far away in the
     * lighting calculations
    */
    for (uint i = 0; i < sceneDataFrag.directionalLightsCount; i++) {
        LightInstanceDataSSBO light = lightInstanceData.instances[i];
        /* Because all the light rays are parallel, it does not matter how each object relates to the light source's
         * position since the light direction remains the same for each object in the scene. And, since the light's
         * direction stays the same, the lighting calculations will be similar for each object in the scene
         *
         * Note that, we expect the light direction to be a direction from the fragment towards the light source, but
         * people generally prefer to specify a directional light with a direction pointing from the light source.
         * Therefore we have to negate the light direction to switch its direction
        */
        vec4 lightDirection = normalize (vec4 (-light.direction, 0.0));
        fragColor          += addLight  (light, lightDirection);
    }
    /* |------------------------------------------------------------------------------------------------------------|
     * | POINT LIGHTS                                                                                               |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* A point light is a light source with a given position somewhere in a world that illuminates in all directions,
     * where the light rays fade out over distance. Think of light bulbs and torches as light casters that act as a
     * point light
    */
    uint pointLightOffset = sceneDataFrag.directionalLightsCount;
    for (uint i = pointLightOffset; i < pointLightOffset + sceneDataFrag.pointLightsCount; i++) {
        LightInstanceDataSSBO light = lightInstanceData.instances[i];
        vec4 lightDirection         = normalize (vec4 (light.position, 1.0) - fragPosition);
        fragColor                  += addLight  (light, lightDirection);
    }
    /* |------------------------------------------------------------------------------------------------------------|
     * | SPOT LIGHTS                                                                                                |
     * |------------------------------------------------------------------------------------------------------------|
    */
    /* A spotlight is a light source that is located somewhere in the environment that, instead of shooting light rays
     * in all directions, only shoots them in a specific direction. The result is that only the objects within a certain
     * radius of the spotlight's direction are lit and everything else stays dark. A good example of a spotlight would
     * be a street lamp or a flashlight
     *
     * A spotlight is represented by a world-space position, a direction and a cut off angle that specifies the radius
     * of the spotlight. For each fragment we calculate if the fragment is between the spotlight's cone (defined by its
     * radius) and if so, we lit the fragment accordingly
    */
    uint spotLightOffset  = pointLightOffset + sceneDataFrag.pointLightsCount;
    for (uint i = spotLightOffset; i < spotLightOffset + sceneDataFrag.spotLightsCount; i++) {
        LightInstanceDataSSBO light = lightInstanceData.instances[i];
        vec4 lightDirection         = normalize (vec4 (light.position, 1.0) - fragPosition);
        /* To create the effect of a smoothly-edged spotlight we want to simulate a spotlight having an inner and an
         * outer cone. We can set the inner cone as the cone defined by the cut off radius mentioned earlier, but we
         * also want an outer cone that gradually dims the light from the inner to the edges of the outer cone
         *
         * To create an outer cone we simply define another cosine value that specifies the outer cone radius. Then, if
         * a fragment is between the inner and the outer cone it should calculate an intensity value between 0.0 and 1.0.
         * If the fragment is inside the inner cone its intensity is equal to 1.0 and 0.0 if the fragment is outside the
         * outer cone
         *
         * We can calculate such a value using the following equation
         *
         *                              I = (theta - outer cone radius) / epsilon
         *
         * Here (epsilon) is the cosine difference between the inner (phi) and the outer cone (gamma). The resulting I
         * value is then the intensity of the spotlight at the current fragment (we're basically interpolating between
         * the outer cosine and the inner cosine based on the theta value)
         *
         * We now have an intensity value that is either negative when outside the spotlight, higher than 1.0 when inside
         * the inner cone, and somewhere in between around the edges. If we properly clamp the values (to make sure the
         * intensity values won't end up outside the [0, 1] range) we won't need an if-else in the fragment shader and we
         * can simply multiply the light components with the calculated intensity value
        */
        float epsilon   = light.innerRadiusCosine - light.outerRadiusCosine;
        float theta     = dot       (lightDirection, normalize (vec4 (-light.direction, 0.0)));
        float intensity = clamp     ((theta - light.outerRadiusCosine) / epsilon, 0.0, 1.0);
        fragColor      += (addLight (light, lightDirection)) * intensity;
    }

    outColor   = vec4 (fragColor, 1.0);
}