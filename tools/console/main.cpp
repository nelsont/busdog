// main.cpp : Defines the entry point for the console application.
//

/*
    Author   : benoit papillault <benoit.papillault@free.fr>
    Creation : 14/03/2001

 */

#include <windows.h>
extern "C" {
    #include <setupapi.h>
    #include <hidsdi.h>
}
#include <stdio.h>
#include <stdlib.h>

#include "service.h"

void printGUID(GUID guid)
{
    /*
    typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;
*/

    printf("{%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X}",
        guid.Data1,guid.Data2,guid.Data3,
        guid.Data4[0],guid.Data4[1],guid.Data4[2],guid.Data4[3],
        guid.Data4[4],guid.Data4[5],guid.Data4[6],guid.Data4[7]);
}

#define INSTALL_FILTER 0
#define UNINSTALL_FILTER 2
#define VIEW_FILTERS 3
#define SHOW_FILTER_INFO (FoundDevice || action == VIEW_FILTERS)

int modfilter(int action, char* hardwareId, char* service)
{
    HDEVINFO hdev;

    GUID hidGuid;

    HidD_GetHidGuid(&hidGuid);
    
    hdev = SetupDiGetClassDevs(NULL /*&hidGuid*/, NULL, 0, DIGCF_ALLCLASSES /*DIGCF_DEVICEINTERFACE*/ | DIGCF_PRESENT);

    if (hdev == INVALID_HANDLE_VALUE )
    {
        printf("error=%x\n",GetLastError());
        return -1;
    }

    if (action == VIEW_FILTERS)
        printf("List of all devices :\n");

    for (DWORD idx=0;;idx++)
    {
        SP_DEVINFO_DATA  devinfo;
        devinfo.cbSize = sizeof(devinfo);

        if (!SetupDiEnumDeviceInfo(hdev,idx,&devinfo))
        {
            if (GetLastError() != ERROR_NO_MORE_ITEMS)
                printf("SetupDiEnumDeviceInfo = %d\n",GetLastError());
            break;
        }

 
        if (action == VIEW_FILTERS)
        {
            printf("SetupDiEnumDeviceInfo ClassGuid=");
            printGUID(devinfo.ClassGuid);
            printf(" DevInst=%X\n",devinfo.DevInst);
        }

        BYTE Buffer[200];
        DWORD BufferSize = 0;
        DWORD DataType;

        bool FoundDevice = false;

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_HARDWAREID  ,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            FoundDevice = strcmp((const char *)Buffer, hardwareId) == 0;

            if (SHOW_FILTER_INFO)
                printf("\tHARDWAREID   = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_DEVICEDESC,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tDEVICEDESC = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_LOWERFILTERS ,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tLOWERFILTERS = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_DRIVER,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tDRIVER = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_ENUMERATOR_NAME,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tENUMERATOR_NAME = %s\n",Buffer);
        }
 
        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_FRIENDLYNAME,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tFRIENDLYNAME = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME   ,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tPHYSICAL_DEVICE_OBJECT_NAME    = %s\n",Buffer);
        }

        if (SetupDiGetDeviceRegistryProperty(hdev,&devinfo,SPDRP_UPPERFILTERS,
            &DataType,Buffer,sizeof(Buffer),&BufferSize))
        {
            if (SHOW_FILTER_INFO)
                printf("\tUPPERFILTERS = %s\n",Buffer);
        }

         HDEVINFO hInterface;
        hInterface = SetupDiGetClassDevs(&devinfo.ClassGuid,NULL,NULL,DIGCF_DEVICEINTERFACE);

        if (action == VIEW_FILTERS)
            for (DWORD idx2=0;;idx2++)
            {

                SP_DEVICE_INTERFACE_DATA data;
                data.cbSize = sizeof(data);

                if (!SetupDiEnumDeviceInterfaces(hInterface,NULL,&devinfo.ClassGuid,idx2,&data))
                {
                    if (GetLastError() != ERROR_NO_MORE_ITEMS)
                        printf("SetupDiEnumDeviceInterfaces = %d\n",GetLastError());
                    break;
                }

                DWORD DetailDataSize = 300;
                char * DetailDataBuffer = new char [DetailDataSize];
                PSP_DEVICE_INTERFACE_DETAIL_DATA DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) DetailDataBuffer;
                DetailData->cbSize = sizeof SP_INTERFACE_DEVICE_DETAIL_DATA;

                if (SetupDiGetDeviceInterfaceDetail(hInterface,&data,DetailData,
                            DetailDataSize,NULL,NULL))
                {
                    printf("\tDevicePath = %s\n",DetailData->DevicePath);
                }
                else
                {
                    printf("SetupDiGetDeviceInterfaceDetail = %d\n",GetLastError());
                }

                delete [] DetailDataBuffer;
            }

        if (FoundDevice)
        {
            if (action == INSTALL_FILTER)
            {
                printf("OK! Installing lower filters\n");

                int len = strlen(service)+2;
                BYTE * LowerFilters = (BYTE *) malloc(len);
                memset(LowerFilters,0,len);
                strcpy((char *)LowerFilters,service);

                if (!SetupDiSetDeviceRegistryProperty(hdev,&devinfo,SPDRP_LOWERFILTERS ,
                            LowerFilters,len))
                {
                    printf("SetupDiSetDeviceRegistryProperty = %d\n",GetLastError());
                }
            }

            if (action == UNINSTALL_FILTER)
            {
                printf("OK! Uninstalling lower filters\n");
                if (!SetupDiSetDeviceRegistryProperty(hdev,&devinfo,SPDRP_LOWERFILTERS ,
                            NULL,0))
                {
                    printf("SetupDiSetDeviceRegistryProperty = %d\n",GetLastError());
                }
            }
        }

     }

    if (GetLastError() != ERROR_NO_MORE_ITEMS)
        printf("error2=%d\n",GetLastError());

    return 0;
}

void usage()
{
    printf("usage: console.exe -i|-u HardwareId ServiceName\n");
    printf("                   -v\n");
    printf("                   -c ServiceName ServiceBinaryPath\n");
    printf("                   -d ServiceName\n");
    printf("  -i : install filter\n");
    printf("  -u : uninstall filter\n");
    printf("  -v : view filters\n");
    printf("  -c : create driver service\n");
    printf("  -d : delete service\n");
    printf("HardwareId MUST match the device hardware id\n");
    printf("ServiceName is the service under which your filter is registered\n");
    printf("ServiceBinaryPath is the fully-qualified path to the service binary file\n");
}

int __cdecl main(int argc, char* argv[])
{
    if (argc < 2)
    {
        usage();
        return -1;
    }

    if (strcmp(argv[1], "-i") == 0 && argc == 4)
        return modfilter(INSTALL_FILTER, argv[2], argv[3]);
    else if (strcmp(argv[1], "-u") == 0 && argc == 4)
        return modfilter(UNINSTALL_FILTER, argv[2], argv[3]);
    else if (strcmp(argv[1], "-v") == 0 && argc == 2)
        return modfilter(VIEW_FILTERS, "", "");
    else if (strcmp(argv[1], "-c") == 0 && argc == 4)
        return CreateSvc(argv[2], argv[3], false);
    else if (strcmp(argv[1], "-d") == 0 && argc == 3)
        return DeleteSvc(argv[2]);
    else
    {
        usage();
        return -1;
    }
}

