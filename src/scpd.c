#include <stdio.h>
#include <stdlib.h>

char *deviceDescriptionXML(const char *deviceName, const char *deviceUUID) {
  const char *format =
      "<?xml version=\"1.0\"?>\n"
      "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n"
      "  <specVersion><major>1</major><minor>0</minor></specVersion>\n"
      "  <device>\n"
      "    "
      "<deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>\n"
      "    <friendlyName>%s</friendlyName>\n"
      "    <manufacturer>go-dlna</manufacturer>\n"
      "    <modelName>mpv-renderer</modelName>\n"
      "    <UDN>%s</UDN>\n"
      "    <serviceList>\n"
      "      <service>\n"
      "        "
      "<serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>\n"
      "        <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>\n"
      "        <controlURL>/ctrl/avt</controlURL>\n"
      "        <eventSubURL>/evt/avt</eventSubURL>\n"
      "        <SCPDURL>/scpd/avt</SCPDURL>\n"
      "      </service>\n"
      "      <service>\n"
      "        "
      "<serviceType>urn:schemas-upnp-org:service:RenderingControl:1</"
      "serviceType>\n"
      "        <serviceId>urn:upnp-org:serviceId:RenderingControl</serviceId>\n"
      "        <controlURL>/ctrl/rc</controlURL>\n"
      "        <eventSubURL>/evt/rc</eventSubURL>\n"
      "        <SCPDURL>/scpd/rc</SCPDURL>\n"
      "      </service>\n"
      "      <service>\n"
      "        "
      "<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</"
      "serviceType>\n"
      "        "
      "<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>\n"
      "        <controlURL>/ctrl/cm</controlURL>\n"
      "        <eventSubURL>/evt/cm</eventSubURL>\n"
      "        <SCPDURL>/scpd/cm</SCPDURL>\n"
      "      </service>\n"
      "    </serviceList>\n"
      "  </device>\n"
      "</root>";

  int size = snprintf(NULL, 0, format, deviceName, deviceUUID);

  char *xml = malloc(size + 1);
  if (xml == NULL) {
    return NULL;
  }

  snprintf(xml, size + 1, format, deviceName, deviceUUID);

  return xml;
}
