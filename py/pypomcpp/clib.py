import ctypes

class CLib():
    loaded_libs = []

    def __init__(self, library_path) -> None:
        try:
            if library_path in CLib.loaded_libs:
                raise ValueError(
                    f"Trying to load {library_path} multiple times. "
                    "This is forbidden to avoid unwanted side effects. "
                    "If you want to load independent instances of the same library, "
                    "create a copy with a different name.")
            
            self.lib = ctypes.cdll.LoadLibrary(library_path)
            CLib.loaded_libs.append(library_path)
        except OSError as err:
            raise ValueError("Could not load library. Reason: " + str(err))
    
    def get_fun(self, name, argtypes, restype):
        """
        Loads a function with the given name from the library and adds the function signature for type checking.

        :param name: The name of the function
        :param argtypes: The argument types
        :param restype: The return type
        """
        lib_fun = self.lib[name]
        lib_fun.restype = restype
        lib_fun.argtypes = argtypes
        return lib_fun
