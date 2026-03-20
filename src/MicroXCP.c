#include "MicroXcp.h"
#include "MicroXcp_private.h"
#include "string.h"

static MicroXcp_Obj_t Xcp_obj = {0};
MicroXcp_Obj_t *this = &Xcp_obj;

MicroXcp_RegisterTable_t CtoTable[] = {
    {.pid = CONNECT,.func = NULL},
    {.pid = DISCONNECT,.func = NULL},
    {.pid = GET_STATUS,.func = NULL},
    {.pid = SYNCH,.func = NULL},

};

static void MicroXcp_CtoInit()
{
    for(int i = 0; i < 3; i++) // 把链表接起来
    {
        this->cto.list[i].next = &this->cto.list[i+1];
    }

    this->cto.list[3].next = NULL; // 不循环，查到NULL就退出


}

static void MicroXcp_RegisterCto()
{

}

void MicroXcp_Init()
{
    MicroXcp_CtoInit();

}

int MICROXCP_WEAK MicroXcp_Transmit(uint8_t *data, size_t size)
{
    (void)data;
    (void)size;

    return 0;
}


MicroXcp_Status_t MicroXcp_TimerHandler()
{

    if(this->frame.byte.pid >= 0xFC && this->frame.byte.pid <= 0xFF) // 查找Cto
    {

    }else { // 查找DTO

    }

    this->ready.en = false;

    return MICROXCP_OK;

}

void MicroXcp_ReceiveCallback(uint8_t *data,size_t len)
{
    if(data == NULL || len == 0 || len > sizeof(this->frame))
    {
        return;
    }

    memcpy(this->frame.data,data,len);
    this->ready.en = true;
    this->ready.len = len;



}


