#
# Copyright (C) 2009 Sun Microsystems, Inc.
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 USA or visit http://www.sun.com if you need
# additional information or have any questions.
#
import sys,os
import traceback

VboxBinDir = os.environ.get("VBOX_PROGRAM_PATH", None)
VboxSdkDir = os.environ.get("VBOX_SDK_PATH", None)

if VboxBinDir is None:
    # Will be set by the installer
    VboxBinDir = "%VBOX_INSTALL_PATH%"

if VboxSdkDir is None:
    VboxSdkDir = VboxBinDir+"/sdk"

os.environ["VBOX_PROGRAM_PATH"] = VboxBinDir
os.environ["VBOX_SDK_PATH"] = VboxSdkDir
sys.path.append(VboxBinDir)

from VirtualBox_constants import VirtualBoxReflectionInfo

class PlatformMSCOM:
    # Class to fake access to constants in style of foo.bar.boo 
    class ConstantFake:
        def __init__(self, parent, name):
            self.__dict__['_parent'] = parent
            self.__dict__['_name'] = name
            self.__dict__['_consts'] = {}
            try:
                self.__dict__['_depth']=parent.__dict__['_depth']+1
            except:
                self.__dict__['_depth']=0
                if self.__dict__['_depth'] > 4:
                    raise AttributeError

        def __getattr__(self, attr):
            import win32com
            from win32com.client import constants
                
            if attr.startswith("__"):
                raise AttributeError

            consts = self.__dict__['_consts']
            
            fake = consts.get(attr, None)
            if fake != None:
               return fake  
            try:
               name = self.__dict__['_name']
               parent = self.__dict__['_parent']
               while parent != None:                  
                  if parent._name is not None:
                    name = parent._name+'_'+name
                  parent = parent._parent
                
               if name is not None:
                  name += "_" + attr
               else:
                  name = attr
               return win32com.client.constants.__getattr__(name)
            except AttributeError,e:
               fake = PlatformMSCOM.ConstantFake(self, attr)
               consts[attr] = fake
               return fake 


    class InterfacesWrapper:
            def __init__(self):
                self.__dict__['_rootFake'] = PlatformMSCOM.ConstantFake(None, None)

            def __getattr__(self, a):
                import win32com
                from win32com.client import constants
                if a.startswith("__"):
                    raise AttributeError
                try:
                    return win32com.client.constants.__getattr__(a)
                except AttributeError,e:
                    return self.__dict__['_rootFake'].__getattr__(a)

    VBOX_TLB_GUID  = '{46137EEC-703B-4FE5-AFD4-7C9BBBBA0259}'
    VBOX_TLB_LCID  = 0
    VBOX_TLB_MAJOR = 1
    VBOX_TLB_MINOR = 0

    def __init__(self, params):
            from win32com import universal
            from win32com.client import gencache, DispatchWithEvents, Dispatch
            from win32com.client import constants, getevents
            import win32com
            import pythoncom
            import win32api
            self.constants = PlatformMSCOM.InterfacesWrapper()                        

    def getSessionObject(self, vbox):
        import win32com
        from win32com.client import Dispatch
	return win32com.client.Dispatch("VirtualBox.Session")

    def getVirtualBox(self):
	import win32com
        from win32com.client import Dispatch
        return win32com.client.Dispatch("VirtualBox.VirtualBox")

    def getConstants(self):
        return self.constants
    
    def getType(self):
        return 'MSCOM'

    def getRemote(self):
        return False

    def getArray(self, obj, field):
        return obj.__getattr__(field)

    def initPerThread(self):
        import pythoncom
        pythoncom.CoInitializeEx(0)

    def deinitPerThread(self):
        import pythoncom
        pythoncom.CoUninitialize()

    def createCallback(self, iface, impl, arg):        
        d = {}
        d['BaseClass'] = impl
        d['arg'] = arg
	d['tlb_guid'] = PlatformMSCOM.VBOX_TLB_GUID
        str = ""
        str += "import win32com.server.util\n"
        #str += "from win32com import universal\n"
        #str += "import pythoncom\n"
        #str += "universal.RegisterInterfaces(tlb_guid, 0, 1, 0, ['"+iface+"'])\n"

        str += "class "+iface+"Impl(BaseClass):\n"
        str += "   _com_interfaces_ = ['"+iface+"']\n"
        str += "   _typelib_guid_ = tlb_guid\n"
	str += "   _typelib_version_ = 1, 0\n"
        #str += "   _reg_clsctx_ = pythoncom.CLSCTX_INPROC_SERVER\n"
        #str += "   _reg_clsid_ = '{F21202A2-959A-4149-B1C3-68B9013F3335}'\n"
        #str += "   _reg_progid_ = 'VirtualBox."+iface+"Impl'\n"
        #str += "   _reg_desc_ = 'Generated callback implementation class'\n"
        #str += "   _reg_policy_spec_ = 'win32com.server.policy.EventHandlerPolicy'\n"

        str += "   def __init__(self): BaseClass.__init__(self, arg)\n"
        str += "result = win32com.server.util.wrap("+iface+"Impl())\n"        
        exec (str,d,d)
        return d['result']

    def waitForEvents(self, timeout):
        from win32file import CloseHandle
        from win32con import DUPLICATE_SAME_ACCESS
	from win32api import GetCurrentThread,DuplicateHandle,GetCurrentProcess
	from win32event import MsgWaitForMultipleObjects, \
                               QS_ALLINPUT, WAIT_TIMEOUT, WAIT_OBJECT_0
        from pythoncom import PumpWaitingMessages

        pid = GetCurrentProcess()
	handle = DuplicateHandle(pid, GetCurrentThread(), pid, 0, 0, DUPLICATE_SAME_ACCESS)
        
        handles = []
	handles.append(handle)

	rc = MsgWaitForMultipleObjects(handles, 0, timeout, QS_ALLINPUT)
        if rc >= WAIT_OBJECT_0 and rc < WAIT_OBJECT_0+len(handles):
            # is it possible?
            print "how come?"
            pass
        elif rc==WAIT_OBJECT_0 + len(handles):
            # Waiting messages
            PumpWaitingMessages()
        else:
            pass
        CloseHandle(handle)

    def deinit(self):
        import pythoncom
        pythoncom.CoUninitialize()
        pass

class PlatformXPCOM:
    def __init__(self, params):
        sys.path.append(VboxSdkDir+'/bindings/xpcom/python/')
        import xpcom.vboxxpcom
        import xpcom
        import xpcom.components

    def getSessionObject(self, vbox):
        import xpcom.components
        return xpcom.components.classes["@virtualbox.org/Session;1"].createInstance()

    def getVirtualBox(self):
        import xpcom.components
        return xpcom.components.classes["@virtualbox.org/VirtualBox;1"].createInstance()

    def getConstants(self):
        import xpcom.components
        return xpcom.components.interfaces
    
    def getType(self):
        return 'XPCOM'
    
    def getRemote(self):
        return False

    def getArray(self, obj, field):
        return obj.__getattr__('get'+field.capitalize())()

    def initPerThread(self):
        pass

    def deinitPerThread(self):
        pass

    def createCallback(self, iface, impl, arg):
        d = {}
        d['BaseClass'] = impl
        d['arg'] = arg
        str = ""
        str += "import xpcom.components\n"
        str += "class "+iface+"Impl(BaseClass):\n"
        str += "   _com_interfaces_ = xpcom.components.interfaces."+iface+"\n"
        str += "   def __init__(self): BaseClass.__init__(self, arg)\n"
        str += "result = "+iface+"Impl()\n"
        exec (str,d,d)
        return d['result']

    def waitForEvents(self, timeout):
        import xpcom
        xpcom._xpcom.WaitForEvents(timeout)

    def deinit(self):
        import xpcom
        xpcom._xpcom.DeinitCOM()

class PlatformWEBSERVICE:
    def __init__(self, params):
        sys.path.append(VboxSdkDir+'/bindings/webservice/python/lib')
        import VirtualBox_services
        import VirtualBox_wrappers
        from VirtualBox_wrappers import IWebsessionManager2
        if params is not None:
            self.user = params.get("user", "")
            self.password = params.get("password", "")
            self.url = params.get("url", "")
        else:
            self.user = ""
            self.password = ""
            self.url = None
        self.wsmgr = IWebsessionManager2(self.url)

    def getSessionObject(self, vbox):
        return self.wsmgr.getSessionObject(vbox)

    def getVirtualBox(self):
        return self.wsmgr.logon(self.user, self.password)

    def getConstants(self):
        return None
    
    def getType(self):
        return 'WEBSERVICE'
    
    def getRemote(self):
        return True

    def getArray(self, obj, field):
        return obj.__getattr__(field)
    
    def initPerThread(self):
        pass

    def deinitPerThread(self):
        pass

    def createCallback(self, iface, impl, arg):
        raise Exception("no callbacks for webservices")

    def waitForEvents(self, timeout):
        # Webservices cannot do that
        pass

    def deinit(self):
        # should we do something about it?
        pass

class SessionManager:
    def __init__(self, mgr):
        self.mgr = mgr

    def getSessionObject(self, vbox):
        return self.mgr.platform.getSessionObject(vbox)

class VirtualBoxManager:
    def __init__(self, style, platparams):
        if style is None:
            if sys.platform == 'win32':
                style = "MSCOM"
            else:
                style = "XPCOM"
        try:
            exec "self.platform = Platform"+style+"(platparams)"
            self.vbox = self.platform.getVirtualBox()
            self.mgr = SessionManager(self)
            self.constants = VirtualBoxReflectionInfo()
            self.type = self.platform.getType()
            self.remote = self.platform.getRemote()
        except Exception,e:
            print "init exception: ",e
            traceback.print_exc()
            raise e

    def getArray(self, obj, field):
        return self.platform.getArray(obj, field)       

    def getVirtualBox(self):
        return  self.platform.getVirtualBox()

    def __del__(self):
        deinit(self)

    def deinit(self):
        if hasattr(self, "vbox"):
            del self.vbox
        if hasattr(self, "platform"):
            self.platform.deinit()

    def initPerThread(self):
        self.platform.initPerThread()

    def openMachineSession(self, machineId):
         session = self.mgr.getSessionObject(self.vbox)
         self.vbox.openSession(session, machineId)
         return session

    def closeMachineSession(self, session):
        session.close()

    def deinitPerThread(self):
        self.platform.deinitPerThread()

    def createCallback(self, iface, impl, arg):
        return self.platform.createCallback(iface, impl, arg)

    def waitForEvents(self, timeout):
        return self.platform.waitForEvents(timeout)
