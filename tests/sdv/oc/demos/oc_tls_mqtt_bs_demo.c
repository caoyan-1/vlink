/**
 * Copyright (c) [2019] maminjie <canpool@163.com>
 *
 * vlink is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *    http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2019>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "vos.h"
#include "oc_mqtt.h"


/* brief : the oceanconnect platform only support the ca_crt up tills now*/
/** the address product_id device_id password crt is only for the test  */

#define DEFAULT_LIFETIME            20
//#define BS_SERVER_ADDRESS            "iot-bs.cn-north-4.myhuaweicloud.com"
#define BS_SERVER_ADDRESS           "119.3.251.30"
#define BS_SERVER_PORT              "8883"           ///<  server mqtt service port
// #define DEMO_WITH_BOOTSTRAP_NODEID  "mqtt_sdk03"
// #define DEMO_WITH_BOOTSTRAP_PASSWORD "f62fcf47d62c4ed18913"//"77dca653824757da0a96" //"e8775e734c48d20aa3ce"
#define DEMO_WITH_BOOTSTRAP_NODEID  "mqtt_test_02"
#define DEMO_WITH_BOOTSTRAP_PASSWORD "53d711c26cf3d03551ff"


#if 0
static char s_mqtt_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIEwTCCAqmgAwIBAgIRdi8oqJITu4uSWV2C/dUS1swwDQYJKoZIhvcNAQELBQAw\r\n"
"PDELMAkGA1UEBhMCQ04xDzANBgNVBAoTBkh1YXdlaTEcMBoGA1UEAxMTSHVhd2Vp\r\n"
"IEVxdWlwbWVudCBDQTAeFw0xNzAyMTYwNjM0MzVaFw00MTEwMDgwNjM0MzVaME0x\r\n"
"CzAJBgNVBAYTAkNOMQ8wDQYDVQQKEwZIdWF3ZWkxLTArBgNVBAMTJEh1YXdlaSBD\r\n"
"bG91ZCBDb3JlIE5ldHdvcmsgUHJvZHVjdCBDQTCCASIwDQYJKoZIhvcNAQEBBQAD\r\n"
"ggEPADCCAQoCggEBAKQQz5uvp3lmtK9uzeje7cZUF8RlRKavEQj9EQk45ly9a/Kr\r\n"
"07TlGIhaZv7j+N9ZV1jTiwA0+XWlni1anjy01qsBQ22eIzX3HQ3fTMjPfk67SFhS\r\n"
"aHdzkJwO66/Nojnaum84HfUTBuXfgiBNH4C2Bc9YBongLktwunqMGvMWEKj4YqjN\r\n"
"bjjZQ1G1Qnhk15qpEWI9YUv0I5X94oT5idEMIH+V+2hcW/6GmztoOgCekW3GPHGl\r\n"
"UJLt3cSaDkp1b5IchnGpwocZLJMd+V3emcLhbjXewIY3meUIkXMrqJ12L3ltkRVp\r\n"
"nHElgmpvp3dBjUrBiITLakrGq8P/Ta7OO5kf9JUCAwEAAaOBrDCBqTAfBgNVHSME\r\n"
"GDAWgBQq+BBZJ4A1H6d8ujufKuRKqpuS6jBGBgNVHSAEPzA9MDsGBFUdIAAwMzAx\r\n"
"BggrBgEFBQcCARYlaHR0cDovL3N1cHBvcnQuaHVhd2VpLmNvbS9zdXBwb3J0L3Br\r\n"
"aTAPBgNVHRMECDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBBjAdBgNVHQ4EFgQU9CcR\r\n"
"7wi0DatgF91OjC2HvAn0bG4wDQYJKoZIhvcNAQELBQADggIBADli3WJezyDe4qVl\r\n"
"jSpF3kxRvBPf6hPhf81WT/A3lo5f7rTOEkRqTB515i5p8Xv8QgP8WTcMu22K5oZt\r\n"
"6iV4PpWCaEVaHgAYeI8sjjWqI498nKWrJ1kaJkdOxB3omVa2Fet7xDCL6MR2jDZq\r\n"
"dtZGF2XCIiNuZhvTYOTvKTfuzN5/gi/z8GD8xP95V4vX3sB2jqgleMTirFdeA+RB\r\n"
"HDbS55MSIfA2jeXJt0IHoB5FKshcCPNLIW/s0O7lbSH62o4d+5PeVV8tbMESQ6Ve\r\n"
"NSRt22+n6Z2Z6n/ZMfM2jSziEZNIyPXQtywltkcrhRIbxWoB0IEr0Ci+7FVr9CRu\r\n"
"ZlcpliSKemrxiLo5EkoznWwxfUP11i473lUVljloJRglYWh6uo9Ci5Cu001occ4L\r\n"
"9qs13MTtpC96LouOyrqBUOlUmJvitqCrHSfqOowyi8B0pxH/+m+Q8+fP9cBztw22\r\n"
"JId8bth5j0OUbNDoY7DnjQLCI0bEZt4RSpQN1c6xf90gHutM62qoGI6NKlb2IH+r\r\n"
"Yfun6P4jYTN9vMnaDfxBH7ofz4q9nj27UBkX9ebqM8kS+RijnUUy8L2N6KsUpp2R\r\n"
"01cjcmp699kFIJ7ShCOmI95ZC9cociTnhTK6WScCweBH7eBxZwoQLi5ER3VkDs1r\r\n"
"rqnNVUgf2j9TOshCI6zuaxsA35wr\r\n"
"-----END CERTIFICATE-----\r\n";
#else
static char s_mqtt_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIID4DCCAsigAwIBAgIJAK97nNS67HRvMA0GCSqGSIb3DQEBCwUAMFMxCzAJBgNV\r\n"
"BAYTAkNOMQswCQYDVQQIEwJHRDELMAkGA1UEBxMCU1oxDzANBgNVBAoTBkh1YXdl\r\n"
"aTELMAkGA1UECxMCQ04xDDAKBgNVBAMTA0lPVDAeFw0xNjA1MDQxMjE3MjdaFw0y\r\n"
"NjA1MDIxMjE3MjdaMFMxCzAJBgNVBAYTAkNOMQswCQYDVQQIEwJHRDELMAkGA1UE\r\n"
"BxMCU1oxDzANBgNVBAoTBkh1YXdlaTELMAkGA1UECxMCQ04xDDAKBgNVBAMTA0lP\r\n"
"VDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAJxM9fwkwvxeILpkvoAM\r\n"
"Gdqq3x0G9o445F6Shg3I0xmmzu9Of8wYuW3c4jtQ/6zscuIGyWf06ke1z//AVZ/o\r\n"
"dp8LkuFbBbDXR5swjUJ6z15b6yaYH614Ty/d6DrCM+RaU+FWmxmOon9W/VELu2BB\r\n"
"NXDQHJBSbWrLNGnZA2erk4JSMp7RhHrZ0QaNtT4HhIczFYtQ2lYF+sQJpQMrjoRn\r\n"
"dSV9WB872Ja4DgcISU1+wuWLmS/NKjIvOWW1upS79yu2I4Rxos2mFy9xxz311rGC\r\n"
"Z3X65ejFNzCUrNgf6NEP1N7wB9hUu7u50aA+/56D7EgjeI0gpFytC+a4f6JCPVWI\r\n"
"Lr0CAwEAAaOBtjCBszAdBgNVHQ4EFgQUcGqy59oawLEgMl21//7F5RyABpwwgYMG\r\n"
"A1UdIwR8MHqAFHBqsufaGsCxIDJdtf/+xeUcgAacoVekVTBTMQswCQYDVQQGEwJD\r\n"
"TjELMAkGA1UECBMCR0QxCzAJBgNVBAcTAlNaMQ8wDQYDVQQKEwZIdWF3ZWkxCzAJ\r\n"
"BgNVBAsTAkNOMQwwCgYDVQQDEwNJT1SCCQCve5zUuux0bzAMBgNVHRMEBTADAQH/\r\n"
"MA0GCSqGSIb3DQEBCwUAA4IBAQBgv2PQn66gRMbGJMSYS48GIFqpCo783TUTePNS\r\n"
"tV8G1MIiQCpYNdk2wNw/iFjoLRkdx4va6jgceht5iX6SdjpoQF7y5qVDVrScQmsP\r\n"
"U95IFcOkZJCNtOpUXdT+a3N+NlpxiScyIOtSrQnDFixWMCJQwEfg8j74qO96UvDA\r\n"
"FuTCocOouER3ZZjQ8MEsMMquNEvMHJkMRX11L5Rxo1pc6J/EMWW5scK2rC0Hg91a\r\n"
"Lod6aezh2K7KleC0V5ZlIuEvFoBc7bCwcBSAKA3BnQveJ8nEu9pbuBsVAjHOroVb\r\n"
"8/bL5retJigmAN2GIyFv39TFXIySw+lW0wlp+iSPxO9s9J+t\r\n"
"-----END CERTIFICATE-----\r\n";
#endif


//if your command is very fast,please use a queue here--TODO
#define cn_app_rcv_buf_len 256
static char     s_rcv_buffer[cn_app_rcv_buf_len];
static int      s_rcv_datalen;
static vsem_t   s_oc_rcv_sync;
static vtask_t  s_report_task, s_cmd_task;

static oc_context_t s_mqtt_handle;

static int app_msg_deal(uintptr_t handle, oc_mqtt_rcv_t *msg)
{
    int ret = -1;
    printf("topic:%s qos:%d\n\r", msg->topic, msg->qos);

    if (msg->data_len < cn_app_rcv_buf_len) {
        memcpy(s_rcv_buffer, msg->data, msg->data_len);
        s_rcv_buffer[msg->data_len] = '\0'; ///< the json need it
        s_rcv_datalen = msg->data_len;

        printf("msg:%s\n\r", s_rcv_buffer);

        vos_sem_post(s_oc_rcv_sync);
        ret = 0;
    }
    return ret;
}

static int oc_mqtt_report_entry(uintptr_t args)
{
    int leftpower = 0;
    oc_json_report_t report;
    oc_json_t lst;
    oc_config_t config = {0};
    cJSON *root = NULL;
    char *buf = NULL;

    int times = 0;

    {
        config.bs_mode = OC_MQTT_BS_MODE_CLIENT_INITIALIZE;
        config.lifetime = DEFAULT_LIFETIME;
        config.host = BS_SERVER_ADDRESS;
        config.port = BS_SERVER_PORT;
        config.dealer = app_msg_deal;
        config.codec_mode = OC_MQTT_CODEC_MODE_JSON;
        config.sign_type = OC_MQTT_SIGN_TYPE_HMACSHA256;
        config.dev_type = OC_MQTT_DEV_TYPE_STATIC;
        config.auth_type = OC_MQTT_AUTH_TYPE_NODEID;
        config.dev_info.s.devid = DEMO_WITH_BOOTSTRAP_NODEID;
        config.dev_info.s.devpwd = DEMO_WITH_BOOTSTRAP_PASSWORD;

        config.security.type = OC_MQTT_SECURTIY_CAS;
        config.security.u.cas.cert = (unsigned char *)s_mqtt_ca_crt;
        config.security.u.cas.cert_len = sizeof(s_mqtt_ca_crt); ///< must including the end '\0'

        if (oc_mqtt_init(&s_mqtt_handle, &config) != 0) {
            printf("config err\r\n");
            return -1;
        }

        leftpower = 33;
        while (1) // do the loop here
        {
            leftpower = (leftpower + 7) % 100;

            lst.key = "batteryVoltage";
            lst.value = (char *)&leftpower;
            lst.len = sizeof(leftpower);
            lst.type = OC_JSON_TYPE_INT;
            lst.next = NULL;

            report.hasmore = 0;
            report.paralst = &lst;
            report.srvid = "DeviceStatus";
            report.eventtime = NULL;

            root = oc_json_fmt_report(&report);
            if (NULL != root) {
                buf = cJSON_Print(root);
                if (NULL != buf) {
                    if (0 == oc_mqtt_report(s_mqtt_handle, buf, strlen(buf), OC_MQTT_QOS_1)) {
                        printf("times:%d power:%d\r\n", times++, leftpower);
                    }
                    vos_free(buf);
                }

                cJSON_Delete(root);
            }

            vos_task_sleep(20 * 1000); ///< do a sleep here
        }
    }
    return 0;
}

static int oc_mqtt_cmd_entry(uintptr_t args)
{
    cJSON *msg = NULL;
    cJSON *mid = NULL;
    cJSON *ioswitch = NULL;
    cJSON *msgType = NULL;
    cJSON *paras = NULL;
    cJSON *serviceId = NULL;
    cJSON *cmd = NULL;
    char *buf = NULL;

    oc_json_rsp_t response;
    oc_json_t list;

    int mid_int;
    int err_int;

    while (1) {
        if (vos_sem_wait(s_oc_rcv_sync) == 0) {
            err_int = 1;
            mid_int = 1;
            msg = cJSON_Parse(s_rcv_buffer);

            if (NULL != msg) {
                serviceId = cJSON_GetObjectItem(msg, "serviceId");
                if (NULL != serviceId) {
                    printf("serviceId:%s\n\r", serviceId->valuestring);
                }

                mid = cJSON_GetObjectItem(msg, "mid");
                if (NULL != mid) {
                    mid_int = mid->valueint;
                    printf("mid:%d\n\r", mid_int);
                }

                msgType = cJSON_GetObjectItem(msg, "msgType");
                if (NULL != msgType) {
                    printf("msgType:%s\n\r", msgType->valuestring);
                }

                cmd = cJSON_GetObjectItem(msg, "cmd");
                if (NULL != cmd) {
                    printf("cmd:%s\n\r", cmd->valuestring);
                }

                paras = cJSON_GetObjectItem(msg, "paras");
                if (NULL != paras) {
                    ioswitch = cJSON_GetObjectItem(paras, "ioswitch");
                    if (NULL != ioswitch) {
                        printf("ioswitch:%d\n\r", ioswitch->valueint);
                        err_int = OC_MQTT_ERR_CODE_NONE;
                    } else {
                        printf("handle the json data as your specific profile\r\n");
                        err_int = OC_MQTT_ERR_CODE_FAIL;
                    }
                }
                cJSON_Delete(msg);

                list.key = "body_para";
                list.value = "body_para";
                list.type = OC_JSON_TYPE_STRING;
                list.next = NULL;

                response.hasmore = 0;
                response.errcode = err_int;
                response.mid = mid_int;
                response.bodylst = &list;

                msg = oc_json_fmt_response(&response);
                if (NULL != msg) {
                    buf = cJSON_Print(msg);
                    if (NULL != buf) {
                        if (0 == oc_mqtt_report(s_mqtt_handle, buf, strlen(buf), OC_MQTT_QOS_1)) {
                            printf("SNDMSG:%s\n\r", buf);
                        }
                        vos_free(buf);
                    }
                    cJSON_Delete(msg);
                }
            }
        }
    }

    return 0;
}

int standard_app_demo_main(void)
{
    printf("bs demo main\r\n");

    vos_sem_init(&s_oc_rcv_sync, 1, 0);

    vos_task_create(&s_report_task, "ocmqtt_report", oc_mqtt_report_entry, 0, 0x1000, 10);
    vos_task_create(&s_cmd_task, "ocmqtt_cmd", oc_mqtt_cmd_entry, 0, 0x1000, 10);

    return 0;
}
