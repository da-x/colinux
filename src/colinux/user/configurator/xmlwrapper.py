from xml.dom import minidom

class XMLWrapper(object):
    def __init__(self, element, type=None):
        self._object = element
        self._type = type

    def __getattr__(self, name):
        if not name.startswith('_'):
            if self._type == 'attr':
                return self._object.getAttribute(name)
            elif self._type == 'sub':
                def generator():
                    for element in self._object.childNodes:
                        if element.nodeName == name:
                            yield XMLWrapper(element)
                return generator()
            else:
                if name in ['attr', 'sub']:
                    return XMLWrapper(self._object, name)
        return super(XMLWrapper, self).__getattribute__(name)

    def parse(file):
        return XMLWrapper(minidom.parse(file))

    parse = staticmethod(parse)
