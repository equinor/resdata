class _BaseCClass:
    def __new__(cls, *args, **kwargs):
        obj = super().__new__(cls)
        # Set safe defaults before __init__ runs, so that a __del__ call on a
        # partially constructed object (e.g. because a subclass __init__
        # raised before calling super().__init__()) does not itself raise an
        # AttributeError - it will just see a None c_pointer and skip free().
        obj.__c_pointer = None
        obj.__parent = None
        obj.__is_reference = False
        return obj

    def __init__(self, c_pointer, parent=None, is_reference=False):
        if not c_pointer:
            raise ValueError("Must have a valid (not null) pointer value!")

        if c_pointer < 0:
            raise ValueError(
                "The pointer value is negative! This may be correct, but usually is not!"
            )

        self.__c_pointer = c_pointer
        self.__parent = parent
        self.__is_reference = is_reference

    def setParent(self, parent):
        self.__parent = parent
        return self

    def _ad_str(self):
        return f"at 0x{self.__c_pointer:x}"

    def _create_repr(self, args=""):
        return f"{self.__class__.__name__}({args}) {self._ad_str()}"

    def free(self):
        raise NotImplementedError(
            "A _BaseCClass requires a free method implementation!"
        )

    def __del__(self):
        if self.free is not None and not self.__is_reference and self.__c_pointer:
            # Important to check the c_pointer; in the case of failed object creation
            # we can have a Python object with c_pointer == None.
            self.free()

    def __repr__(self):
        return self._create_repr()

    def __eq__(self, other):
        if not isinstance(other, _BaseCClass):
            return NotImplemented
        return self.__c_pointer == other.__c_pointer

    def __hash__(self):
        return hash(self.__c_pointer)

    @classmethod
    def createPythonObject(cls, c_pointer):
        if c_pointer is not None:
            new_obj = cls.__new__(cls)
            _BaseCClass.__init__(
                new_obj, c_pointer=c_pointer, parent=None, is_reference=False
            )
            return new_obj
        else:
            return None

    @classmethod
    def createCReference(cls, c_pointer, parent=None):
        if c_pointer is not None:
            new_obj = cls.__new__(cls)
            _BaseCClass.__init__(
                new_obj, c_pointer=c_pointer, parent=parent, is_reference=True
            )
            return new_obj
        else:
            return None


__all__ = ["_BaseCClass"]
