
/********************************** (C) COPYRIGHT *******************************
* File Name          :HID.C
* Author             : 
* Version            :
* Date               :
* Description        :CH554ģ��HID�����豸��֧���ж����´���֧������ȫ�٣����� 
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "CH554.h"
#include "Debug.h"


#define THIS_ENDP0_SIZE         DEFAULT_ENDP0_SIZE

UINT8X	Ep0Buffer[THIS_ENDP0_SIZE] _at_ 0x0000;                                //�˵�0 OUT&IN��������������ż��ַ
UINT8X	Ep2Buffer[2*MAX_PACKET_SIZE] _at_ 0x0008;                              //�˵�2 IN&OUT������,������ż��ַ
UINT8X  Ep1Buffer[MAX_PACKET_SIZE] _at_ 0x00a0;



UINT8   SetupReq,SetupLen,Ready,Count,FLAG,UsbConfig;
PUINT8  pDescr;                                                                    //USB���ñ�־
USB_SETUP_REQ   SetupReqBuf;                                                       //�ݴ�Setup��
UINT8   num = 0;
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)  

sbit Ep2InKey = P1^5;                                                              //K1����
#pragma  NOAREGS
/*�豸������*/
 
UINT8C DevDesc[18]={0x12,0x01,0x10,0x01,0xff,0x00,0x02,0x08,                   //�豸������
                    0x86,0x1a,0x23,0x55,0x04,0x03,0x00,0x00,
                    0x00,0x01};

UINT8C CfgDesc[39]={0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //�������������ӿ�������,�˵�������
	                  0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,           
                    0x07,0x05,0x82,0x02,0x20,0x00,0x00,                        //�����ϴ��˵�
		                0x07,0x05,0x02,0x02,0x20,0x00,0x00,                        //�����´��˵�      
			              0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //�ж��ϴ��˵�

UINT8C DataBuf[26]={0x30,0x00,0xc3,0x00,0xff,0xec,0x9f,0xec,0xff,0xec,0xdf,0xec,
                    0xdf,0xec,0xdf,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,0x9f,0xec,
                    0xff,0xec};

/*�ַ��������� ��*/ 

// unsigned char  code LangDes[]={0x04,0x03,0x09,0x04};           //����������
// unsigned char  code SerDes[]={
//                           0x28,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                           0x00,0x00,0x00,0x00,0x00,0x49,0x00,0x43,0x00,0x42,
//                           0x00,0x43,0x00,0x31,0x00,0x00,0x00,0x00,0x00,0x00
//                           };                                   //�ַ���������

UINT8X UserEp2Buf[64];                                            //�û����ݶ���


/*******************************************************************************
* Function Name  : Enp2BlukIn()
* Description    : USB�豸ģʽ�˵�2�������ϴ�
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Enp2BlukIn( )
{
    memcpy( Ep2Buffer+MAX_PACKET_SIZE, UserEp2Buf, sizeof(UserEp2Buf));        //�����ϴ�����
    UEP2_T_LEN = THIS_ENDP0_SIZE;                                              //�ϴ���������
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                  //������ʱ�ϴ����ݲ�Ӧ��ACK
    while(UEP2_CTRL&UEP_T_RES_ACK);                                            //�ȴ��������
}

/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB�жϴ�������
*******************************************************************************/
void    DeviceInterrupt( void ) interrupt INT_NO_USB using 1                    //USB�жϷ������,ʹ�üĴ�����1
{
    UINT8 len,i;
    if(UIF_TRANSFER)                                                            //USB������ɱ�־
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# �˵������ϴ�
             UEP2_T_LEN = 0;                                                    //Ԥʹ�÷��ͳ���һ��Ҫ���
//            UEP1_CTRL ^= bUEP_T_TOG;                                          //����������Զ���ת����Ҫ�ֶ���ת
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Ĭ��Ӧ��NAK
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 2# �˵������´�
            if ( U_TOG_OK )                                                     // ��ͬ�������ݰ�������
            {
                len = USB_RX_LEN;                                               //�������ݳ��ȣ����ݴ�Ep2Buffer�׵�ַ��ʼ���
                for ( i = 0; i < len; i ++ )
                {
                    Ep2Buffer[MAX_PACKET_SIZE+i] = Ep2Buffer[i];         // OUT����ȡ����IN�ɼ������֤
                }
                UEP2_T_LEN = len;
                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;       // �����ϴ�
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                               //SETUP����
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthL;
                if(UsbSetupBuf->wLengthH || SetupLen > 0x7F )
                {
                    SetupLen = 0x7F;                                             // �����ܳ���
                }
                len = 0;                                                         // Ĭ��Ϊ�ɹ������ϴ�0����
                SetupReq = UsbSetupBuf->bRequest;							
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )/*HID������*/
                {
					switch( SetupReq )                                             
					{
						case 0xC0:                                                  
						  pDescr = &DataBuf[num];
						  len = 2;
						  if(num<24)
						  {	
							num += 2;
							}
							else
							{
								num = 24;
							}											
								 break;
						case 0x40:
							len = 9;   //��֤״̬�׶Σ�����ֻҪ��8���Ҳ�����0xff����
							break;
						default:
								 len = 0xFF;  				                                   /*���֧��*/					
								 break;
					}
					if ( SetupLen > len )
					{
						SetupLen = len;    //�����ܳ���
					}
					len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//���δ��䳤��
					memcpy(Ep0Buffer,pDescr,len);                            //�����ϴ�����
					SetupLen -= len;
					pDescr += len;
						
                }
                else                                                             //��׼����
                {
                    switch(SetupReq)                                             //������
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                  //�豸������
                            pDescr = DevDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                  //����������
                            pDescr = CfgDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(CfgDesc);
                            break;
                        default:
                            len = 0xff;                                          //��֧�ֵ�������߳���
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //�����ܳ���
                        }
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//���δ��䳤��
                        memcpy(Ep0Buffer,pDescr,len);                            //�����ϴ�����
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                         //�ݴ�USB�豸��ַ
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                      //Clear Feature
                        if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// �˵�
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;                                       // ��֧�ֵĶ˵�
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                           // ���Ƕ˵㲻֧��
                        }
                        break;
                    case USB_SET_FEATURE:                                         /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )        /* �����豸 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* ���û���ʹ�ܱ�־ */
                                }
                                else
                                {
                                    len = 0xFF;                                    /* ����ʧ�� */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                        /* ����ʧ�� */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )    /* ���ö˵� */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* ���ö˵�2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF;                                     /* ����ʧ�� */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                         /* ����ʧ�� */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                             /* ����ʧ�� */
                        } 
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                  //����ʧ��
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                          //�����ȴ���
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= THIS_ENDP0_SIZE)                                         //�ϴ����ݻ���״̬�׶η���0���Ȱ�
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1������Ӧ��ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1,����Ӧ��ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;     //���δ��䳤��
                memcpy( Ep0Buffer, pDescr, len );                                   //�����ϴ�����
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                            //ͬ����־λ��ת
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ���
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            len = USB_RX_LEN;
            if(SetupReq == 0x09)
            {
                if(Ep0Buffer[0])
                {
                    printf("Light on Num Lock LED!\n");
                }
                else if(Ep0Buffer[0] == 0)
                {
                    printf("Light off Num Lock LED!\n");
                }
            }
            UEP0_T_LEN = 0;  //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
            UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA0,����Ӧ��ACK
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //д0����ж�
    }
    if(UIF_BUS_RST)                                                                 //�豸ģʽUSB���߸�λ�ж�
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        UIF_BUS_RST = 0;                                                             //���жϱ�־
    }
    if (UIF_SUSPEND)                                                                 //USB���߹���/�������
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //����
        {
#ifdef DE_PRINTF
            printf( "zz" );                                                          //˯��״̬
#endif
            while ( XBUS_AUX & bUART0_TX )
            {
                ;    //�ȴ��������
            }
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                   //USB����RXD0���ź�ʱ�ɱ�����
            PCON |= PD;                                                               //˯��
            SAFE_MOD = 0x55;
            SAFE_MOD = 0xAA;
            WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //������ж�,�����ܷ��������
        USB_INT_FG = 0xFF;                                                             //���жϱ�־
//      printf("UnknownInt  N");
    }
}
/*******************************************************************************
* Function Name  : USBDeviceCfg()
* Description    : USB�豸ģʽ����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceCfg()
{
    USB_CTRL = 0x00;                                                           //���USB���ƼĴ���
    USB_CTRL &= ~bUC_HOST_MODE;                                                //��λΪѡ���豸ģʽ
    USB_CTRL |=  bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                    //USB�豸���ڲ�����ʹ��,���ж��ڼ��жϱ�־δ���ǰ�Զ�����NAK
    USB_DEV_AD = 0x00;                                                         //�豸��ַ��ʼ��
//     USB_CTRL |= bUC_LOW_SPEED;
//     UDEV_CTRL |= bUD_LOW_SPEED;                                                //ѡ�����1.5Mģʽ
    USB_CTRL &= ~bUC_LOW_SPEED;
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                             //ѡ��ȫ��12Mģʽ��Ĭ�Ϸ�ʽ
	  UDEV_CTRL = bUD_PD_DIS;  // ��ֹDP/DM��������
    UDEV_CTRL |= bUD_PORT_EN;                                                  //ʹ�������˿�
}
/*******************************************************************************
* Function Name  : USBDeviceEndPointCfg()
* Description    : USB�豸ģʽ�˵����ã�ģ�����HID�豸�����˶˵�0�Ŀ��ƴ��䣬�������˵�2�������´�
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceEndPointCfg()
{
	UEP1_DMA = Ep1Buffer;                                                      //�˵�1 �������ݴ����ַ
    UEP2_DMA = Ep2Buffer;                                                      //�˵�2 IN���ݴ����ַ	
    UEP2_3_MOD = 0xCC;                                                         //�˵�2/3 �������շ�ʹ��
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //�˵�2�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK

    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK;                                 //�˵�1�Զ���תͬ����־λ��IN���񷵻�NAK	
	UEP0_DMA = Ep0Buffer;                                                      //�˵�0���ݴ����ַ
    UEP4_1_MOD = 0X40;                                                         //�˵�1�ϴ����������˵�0��64�ֽ��շ�������
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //�ֶ���ת��OUT���񷵻�ACK��IN���񷵻�NAK
}
/*******************************************************************************
* Function Name  : USBDeviceIntCfg()
* Description    : USB�豸ģʽ�жϳ�ʼ��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceIntCfg()
{
    USB_INT_EN |= bUIE_SUSPEND;                                               //ʹ���豸�����ж�
    USB_INT_EN |= bUIE_TRANSFER;                                              //ʹ��USB��������ж�
    USB_INT_EN |= bUIE_BUS_RST;                                               //ʹ���豸ģʽUSB���߸�λ�ж�
    USB_INT_FG |= 0x1F;                                                       //���жϱ�־
    IE_USB = 1;                                                               //ʹ��USB�ж�
    EA = 1;                                                                   //������Ƭ���ж�
}

main()
{
    UINT8 i;
    CfgFsys( );                                                           //CH559ʱ��ѡ������
    mDelaymS(5);                                                          //�޸���Ƶ�ȴ��ڲ������ȶ�,�ؼ�	
    mInitSTDIO( );                                                        //����0��ʼ��
#ifdef DE_PRINTF
    printf("start ...\n");
#endif	
    USBDeviceCfg();                                                    
    USBDeviceEndPointCfg();                                               //�˵�����
    USBDeviceIntCfg();                                                    //�жϳ�ʼ��
	UEP0_T_LEN = 0;
    UEP1_T_LEN = 0;                                                       //Ԥʹ�÷��ͳ���һ��Ҫ���
    UEP2_T_LEN = 0;                                                       //Ԥʹ�÷��ͳ���һ��Ҫ���

    while(1)
    {

    }
}