from common import *

class NetworkDevicesOptionArray(OptionArrayPanel):
    CLASS_DESC = 'Virtual Network Adapters'
    SHORT_TREE_DESC = 'Networking'

    def get_item_list(self):
        return self.mainframe.network_devices

    class Item(ConfigurationItem):
        def title(self, long=False):
            device = self._xml_item
            index = int(device.attr.index)
            if long:
                name = 'conet%d' % (index, )
            else:
                name = '[%d]' % (index, )
            return '%s: %s' % (name, device.attr.type)

    class ItemPanel(OptionArrayPanel.ItemPanel):
        pass
