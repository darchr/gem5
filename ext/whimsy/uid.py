import os

import config

def uid(testitem, filepath, class_name=None):
    '''
    The generic function used to produce uid of test objects.
    '''
    # Trim the file path to be the path relative to the parent of this
    # directory.
    filepath = os.path.relpath(filepath,
                               os.path.commonprefix((config.constants.testing_base,
                                                    filepath)))
    fmt = '{file}:{class_}:{name}'
    if class_name is None:
        class_name = testitem.__class__.__name__
    return fmt.format(file=filepath, name=testitem.name,
                      class_=class_name)

# FIXME: Should merge UID functions into a full blown class.
def path_from_uid(uid):
    split_path = uid.split(':')[0]
    return os.path.join(config.constants.testing_base, split_path)
