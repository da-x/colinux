from xml.dom import minidom

class XMLSubWrapper(object):
    def __init__(self, iterobj, object):
        self._iterobj = iterobj
        self.object = object

    def __iter__(self, *ar, **kw):
        return self._iterobj.__iter__(*ar, **kw)

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
                return XMLSubWrapper(generator(), self._object)
            else:
                if name in ['attr', 'sub']:
                    return XMLWrapper(self._object, name)
        return super(XMLWrapper, self).__getattribute__(name)

    def __setattr__(self, name, value):
        if not name.startswith('_'):
            if self._type == 'attr':
                return self._object.setAttribute(name, value)
            elif self._type == 'sub':
                def generator():
                    for element in self._object.childNodes:
                        if element.nodeName == name:
                            yield XMLWrapper(element)
                return XMLSubWrapper(generator(), self._object)
            else:
                if name in ['attr', 'sub']:
                    return XMLWrapper(self._object, name)
        return super(XMLWrapper, self).__setattr__(name, value)

    def build_string(self):
        return self._object.toprettyxml()

    def parse(file):
        return XMLWrapper(minidom.parse(file))

    def striptext(self):
        def recurse(element):
            for sub in list(element.childNodes):
                if sub.__class__ is minidom.Text:
                    element.removeChild(sub)
                recurse(sub)
        return recurse(self._object)

    parse = staticmethod(parse)

    def parse_string(string):
        return XMLWrapper(minidom.parseString(string))

    parse_string = staticmethod(parse_string)
