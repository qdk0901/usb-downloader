#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include "usb.h"

static int debug = 0;
#define INFO(fmt,...) printf("\e[1;33m" fmt "\e[0m\n",##__VA_ARGS__)
#define ERR(fmt,...) printf("\e[1;31m" fmt "\e[0m\n",##__VA_ARGS__);
#define SUCC(fmt,...) printf("\e[1;32m" fmt "\e[0m\n",##__VA_ARGS__);


static int match_stage1(usb_ifc_info *ifc)
{
	if(ifc->dev_vendor == 0x04e8 && ifc->dev_product == 0x1234)
		return 0;
	return -1;
}
static int match_stage2(usb_ifc_info *ifc)
{
	if(ifc->dev_vendor == 0x04e8 && ifc->dev_product == 0x1235)
		return 0;
	return -1;
}
static int match_uboot(usb_ifc_info *ifc)
{
	if(ifc->dev_vendor == 0x18d2 && ifc->dev_product == 0x0005)
		return 0;
	return -1;
}
static unsigned short check_sum(char* buffer,int size)
{	
	int i;
	unsigned short cs=0;
	for(i=0;i<size;i++){
		cs += buffer[i];
	}
	return cs;
}
static int usb_dl(usb_handle* dev,char* data,int size,int addr)
{
	char* buffer;
	int n;

	buffer = (char*)malloc(size+10);
	if(!buffer){
		ERR("allocate buffer failed");
		return -1;
	}

	*((int*)buffer + 0) = addr;
	*((int*)buffer + 1) = size + 10;
	memcpy(&buffer[8],data,size);
	*((short*)(buffer + 8 + size)) = check_sum(&buffer[8],size);

	n = usb_write(dev,buffer,size + 10);

	INFO("download file:%d bytes written",n);
	free(buffer);
	return (n - 10);
}

char* load_file(char* fn,int* _sz)
{
	char *data;
	int sz;
	FILE* f;

	data = 0;
	f = fopen(fn, "rb");
	if(!f) return 0;

	fseek(f , 0 , SEEK_END);
	sz = ftell(f);
	rewind( f );
	if(sz < 0) goto oops;

	if(fseek(f, 0, SEEK_SET) != 0) goto oops;

	data = (char*) malloc(sz);
	if(data == 0) goto oops;

	int n =fread(data,1, sz,f);
	if(n != sz) goto oops;
	fclose(f);

	if(_sz) *_sz = sz;
	return data;

oops:
	fclose(f);
	if(data != 0) free(data);
	return 0;	
}
usb_handle* get_device(void* callback)
{
	usb_handle* h;
	do{
		h = usb_open(callback);
		usleep(100000);
	}while(!h);
	return h;	
}
int dl_file(int stage,char* fn,unsigned long addr)
{
	usb_handle* h;
	void* callback;
	char* data;
	int sz = 0,ret = 0;

	if(stage == 1){
		callback = match_stage1;
	}
	else if(stage == 2)
		callback = match_stage2;
	else
		return -1;

	INFO("#########download %s#########",fn);
	INFO("wait for device to connect");
	
	h = get_device(callback);
	if(!h){
		ERR("cannot open device\n");
		return -1;	
	}
	data = load_file(fn,&sz);
	if(!data){
		ERR("cannot open %s",fn);
		return -1;	
	}
	ret = usb_dl(h,data,sz,addr);
	free(data);
	usb_close(h);
	if(ret != sz){
		ERR("download failed. total:%d,written:%d",sz,ret);
		return -1;
	}
	SUCC("#########success#########");
	return 0;
}
static void usage()
{
	ERR("usage:usbdl <bl2 path> <uboot path>");
}
int main(int argc, char **argv)
{
	if(argc < 3){
		usage();
		return -1;
	}
	if(dl_file(1,argv[1],0xd0020010) < 0)
		return -1;
	if(dl_file(2,argv[2],0x23e00000) < 0)
		return -1;
	return 0;
}



















