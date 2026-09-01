class _BaseCClass:
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

    def __repr__(self):
        return self._create_repr()

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
