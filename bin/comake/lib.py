import sre, os

def normal_path(path):
    parts = path.split(os.path.sep)
    index = 0
    while index < len(parts):
        if parts[index] == '..' and index > 0:
            del parts[index]
            del parts[index-1]
            index -= 1
        else:
            index += 1
    return os.path.sep.join(parts)
