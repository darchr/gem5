def overrides(interface_class):
    """
    Function override annotation.
    Corollary to @abc.abstractmethod where the override is not of an
    abstractmethod.
    Modified from answer https://stackoverflow.com/a/8313042/471376
    """

    def confirm_override(method):
        if method.__name__ not in dir(interface_class):
            raise NotImplementedError(
                f"function {method.__name__} is an @override, but that"
                f" function is not implemented in base class {interface_class}"
            )

        def func():
            pass

        attr = getattr(interface_class, method.__name__)
        if type(attr) is not type(func):
            raise NotImplementedError(
                f"function {method.__name__} is an @overide, but that is"
                f" implemented as type {type(attr)} in base class"
                f" {interface_class}, expected implemented type {type(func)}."
            )
        return method

    return confirm_override
