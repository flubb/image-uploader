#include "MemberFunctionCallback.h"

CBTHookCallbackBase* AvailableCallbackSlots[kMaxCallbacks] = {
    new DynamicCBTHookCallback<0x00>(),
    new DynamicCBTHookCallback<0x01>(),
    new DynamicCBTHookCallback<0x02>(),
    new DynamicCBTHookCallback<0x03>(),
    new DynamicCBTHookCallback<0x04>()
    /*new DynamicCBTHookCallback<0x05>(),
    new DynamicCBTHookCallback<0x06>(),
    new DynamicCBTHookCallback<0x07>(),
    new DynamicCBTHookCallback<0x08>(),
    new DynamicCBTHookCallback<0x09>(),
    new DynamicCBTHookCallback<0x0A>(),
    new DynamicCBTHookCallback<0x0B>(),
    new DynamicCBTHookCallback<0x0C>(),
    new DynamicCBTHookCallback<0x0D>(),
    new DynamicCBTHookCallback<0x0E>(),
    new DynamicCBTHookCallback<0x0F>(),*/
};


CBTHookMemberFunctionCallback::CBTHookMemberFunctionCallback(CWebViewWindow* instance, LPFN_CBTHookMemberFunctionCallback method)
{
    int imax = sizeof(AvailableCallbackSlots)/sizeof(AvailableCallbackSlots[0]);
    for( m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex )
    {
        m_cbCallback = AvailableCallbackSlots[m_nAllocIndex]->Reserve(instance, method);
        if( m_cbCallback != NULL )
            break;
    }
}

CBTHookMemberFunctionCallback::~CBTHookMemberFunctionCallback()
{
    if( IsValid() )
    {
        AvailableCallbackSlots[m_nAllocIndex]->Free();
    }
}