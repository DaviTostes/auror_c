#ifndef SCPD_H
#define SCPD_H

char *deviceDescriptionXML(const char *deviceName, const char *deviceUUID);
const char *avtSCPDXML();
const char *rcSCPDXML();
const char *cmSCPDXML();

#endif /* SCPD_H */
