from _neuropil import lib as neuropil, ffi

class NeuropilException(Exception):
    def __init__(self, message, error):        
        super().__init__(message)
        self.error = error

class Neuropil(object):        
    # ffi interaction variables
    _ffi_handle = None
    _settings = None    
    _context = None   
    # python class variables
    _userdata = None
    _destroyed = False
    # default aaa callbacks
    _user_authn_cb = lambda s,x: True # Default return True 
    _user_authz_cb = lambda s,x: True # Default return True
    _user_accou_cb = lambda s,x: True # Default return True
    # user subject callbacks    
    __callback_info_dict__ = {}


    def __init__(self,throw_exceptions = True):
        self.throw_exceptions = throw_exceptions
        self._ffi_handle = ffi.new_handle(self)        
        self._settings = neuropil.np_default_settings(ffi.NULL)                            
        self._context = neuropil.np_new_context(self._settings)        
        neuropil.np_set_userdata(self._context, self._ffi_handle)
    
    def __del__(self):
        if not self._destroyed:
            self._destroyed = True             
            neuropil.np_destroy(self._context, False)

    def shutdown(self): 
        ret = neuropil.np_ok       
        if not self._destroyed:
            self._destroyed = True
            ret = neuropil.np_destroy(self._context, True)
        return ret

    @staticmethod
    @ffi.callback("bool(np_context* context, struct np_message*)")
    def py_subject_callback(context, message):
        myself = Neuropil.from_context(context)
        
        if myself.__callback_info_dict__[message.subject]:
            for user_fn in myself.__callback_info_dict__[message.subject]:
                user_fn(myself, message)

        return True

    def set_receive_cb(self, subject, recv_callback):        
        ret = neuropil.np_ok
        if subject not in self.__callback_info_dict__:            
            self.__callback_info_dict__[subject] = []
            ret = neuropil.np_add_receive_cb(self._context, subject, Neuropil.py_subject_callback)            
        self.__callback_info_dict__[subject].append(recv_callback)                    
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def listen(self, protocol, hostname, port):
        ret = neuropil.np_listen(self._context, protocol, hostname, port)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def join(self, connect_string):
        ret = neuropil.np_join(self._context, connect_string)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret
            
    def run(self, interval):
        ret = neuropil.np_run(self._context, interval)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def send(self, subject, message):
        raw_bytes = ffi.from_buffer(message)        
        ret = neuropil.np_send(self._context, subject, raw_bytes, len(raw_bytes))
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def new_identity(self, expires_at, secret_key):
        ret = neuropil.np_new_identity(self._context, expires_at, secret_key)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret
    def use_identity(self, identity):
        ret = neuropil.np_use_identity(self._context, identity)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def get_mx_properties(self, subject):    
        ret = neuropil.np_get_mx_properties(self._context, subject)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret
	    
    def set_userdata(self, userdata):
        self._userdata.data = userdata
    def get_userdata(self):
        return self._userdata.data
    
    def has_joined(self):
        ret = neuropil.np_has_joined(self._context);		
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def get_status(self):
        ret = neuropil.np_get_status(self._context);        
        return ret
    
    @staticmethod
    def from_context(context):
        return ffi.from_handle(neuropil.np_get_userdata(context))
        
    @staticmethod
    @ffi.callback("bool(np_context* context, struct np_token*)")
    def _py_authn_cb(context, token):        
        myself = Neuropil.from_context(context)
        return myself._user_authn_cb(token)

    @staticmethod
    @ffi.callback("bool(np_context* context, struct np_token*)")
    def _py_authz_cb(context, token):
        myself = Neuropil.from_context(context)        
        return myself._user_authz_cb(token)
        
    @staticmethod
    @ffi.callback("bool(np_context* context, struct np_token*)")
    def _py_acc_cb(context, token):
        myself = Neuropil.from_context(context)        
        return myself._user_accou_cb(token)

    def set_authenticate_cb(self, authn_callback):
        self._user_authn_cb = authn_callback
        ret =  neuropil.np_set_authenticate_cb(self._context, Neuropil._py_authn_cb)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def set_authorize_cb(self, authz_callback):
        self._user_authz_cb = authz_callback
        ret = neuropil.np_set_authorize_cb(self._context, Neuropil._py_authz_cb)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

    def set_accounting_cb(self, acc_callback):
        self._user_acc = acc_callback
        ret = neuropil.np_set_accounting_cb(self._context, Neuropil._py_acc_cb)
        if self.throw_exceptions and ret is not neuropil.np_ok:
            raise NeuropilException('{error}'.format(error=ffi.string(neuropil.np_error_str[ret])),ret)
        return ret

