#ifndef INSTANCE_MGR_H
#define INSTANCE_MGR_H

#include "Constants.h"
#include <iostream>
#include <map>
#include <cassert>

namespace Collections {
namespace Admin {
    /* A template which can generate a family of types, such as  Buffer <int> or List <double>, all these variants are 
     * not related such that the one is somehow derived from the other or such. So you have to establish some relation 
     * between all these generated types in order store them in a single container (map). One way is to use a common non
     * template base class
     */
    class NonTemplateBase {
        public:
            /* the destructor is virtual for the base class because if you did not have a virtual destructor and through 
             * the pointer to base class when you call destructor you end up calling base class destructor. In this case 
             * you want polymorphism to work on your destructor as well, e.g. through calling destructor on your base class
             * you want to end up calling destructor of your most derived class not JUST your base class.
            */
            virtual ~NonTemplateBase (void) = 0;
    };
    /* pure virtual Destructors must be defined, which is against the pure virtual behaviour. The only difference between 
     * Virtual and Pure Virtual Destructor is, that pure virtual destructor will make its Base class Abstract, hence you 
     * cannot create object of that class (hence why we are doing it). We need an implementation here because If you derive
     * anything from base (UPCASTING) and then try to delete or destroy it, base's destructor will eventually be called. 
     * Since it is pure and doesn't have an implementation, will cause compilation error
    */
    inline NonTemplateBase::~NonTemplateBase (void) {} 

    // all managers will be derived from this class, which allows us to have multiple instances
    class InstanceMgr {
        protected:
            std::map <size_t, NonTemplateBase*> m_instancePool;

        public:
            ~InstanceMgr (void) {
                closeAllInstances();
            }

            NonTemplateBase* getInstance (size_t instanceId) {
                if (m_instancePool.find (instanceId) != m_instancePool.end())
                    return m_instancePool[instanceId];

                // invalid instance id
                else
                    assert (false);
            }

            void closeInstance (size_t instanceId) {
                if (m_instancePool.find (instanceId) != m_instancePool.end()) {
                    delete m_instancePool[instanceId];
                    // remove from map, so you are able to reuse the instance id
                    m_instancePool.erase (instanceId);       
                }
                // closing an instance that doesn't exist, do nothing
                else
                    ;
            }

            void closeAllInstances (void) {
                for (auto const& [key, val] : m_instancePool)
                    delete m_instancePool[key];

                // clear all entries in pool
                m_instancePool.clear();
            }   

            /* instance mgr is displayed in the following format
             * mgr :    
             *      {                                           <L1>
             *          instance count : ?                      <L2>
             *          instances :                         
             *                      {                           <L3>
             *                          id : ?                  <L4>
             *                          ? : ?       
             *                      }                           <L3>
             *                      {
             *                          id : ?
             *                          ? : ?
             *                      }                           <L3>
             *                      ...
             *      }                                           <L1>
            */
            void dump (std::ostream& ost, 
                       void (*lambda) (NonTemplateBase*, std::ostream&) = 
                       [](NonTemplateBase* instance, std::ostream& ost) {
                            ost << TAB_L4 << "address : " << instance << "\n";
                       }) {

                ost << "mgr : " << "\n";
                ost << OPEN_L1;

                ost << TAB_L2 << "instances count : "   << m_instancePool.size() << "\n";
                ost << TAB_L2 << "instances : "         << "\n";

                for (auto const& [key, val] : m_instancePool) {
                    ost << OPEN_L3;
                    ost << TAB_L4   << "id : "          << key  << "\n";
                    /* dump more information about each instance ids through lambda. Since the mgr itself doesn't have
                     * much information unless we cast it back to original type, by default the lambda dumps out the
                     * instance address
                    */
                    lambda (val, ost);  ost << "\n";
                    ost << CLOSE_L3;
                }

                ost << CLOSE_L1;
            }
    };
}   // namespace Admin
}   // namespace Collections
#endif  // INSTANCE_MGR_H