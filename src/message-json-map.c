/*
** Copyright (C) 2009-2019 Quadrant Information Security <quadrantsec.com>
** Copyright (C) 2009-2019 Champ Clark III <cclark@quadrantsec.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Read data from fifo in a JSON format */

#ifdef HAVE_CONFIG_H
#include "config.h"             /* From autoconf */
#endif

#ifndef HAVE_LIBFASTJSON
libfastjson is required for Sagan to function!
#endif


#ifdef HAVE_LIBFASTJSON

#include <stdio.h>
#include <string.h>
#include <json.h>

#include "sagan.h"
#include "sagan-defs.h"
#include "sagan-config.h"
#include "version.h"
#include "message-json-map.h"

#include "parsers/parsers.h"


struct _SaganConfig *config;
struct _SaganCounters *counters;
struct _SaganDebug *debug;

struct _JSON_Message_Map *JSON_Message_Map;
struct _JSON_Message_Tmp *JSON_Message_Tmp;

/*************************
 * Load JSON mapping file
 *************************/

void Load_Message_JSON_Map ( const char *json_map )
{

    struct json_object *json_obj = NULL;
    struct json_object *tmp = NULL;

    char *ptr1 = NULL;
    char *ptr2 = NULL;
    char *data = NULL;

    FILE *json_message_map_file;
    char json_message_map_buf[10240] = { 0 };

    Sagan_Log(NORMAL, "Loading JSON 'message' mapping for '%s'", config->json_message_map_file);

    /* Zero out the array */

    memset(JSON_Message_Map, 0, sizeof(_JSON_Message_Map));

    if (( json_message_map_file = fopen(json_map, "r" )) == NULL )
        {
            Sagan_Log(ERROR, "[%s, line %d] Cannot open JSON map file (%s)", __FILE__, __LINE__, json_map);
        }

    while(fgets(json_message_map_buf, 10240, json_message_map_file) != NULL)
        {

            /* Skip comments and blank lines */

            if (json_message_map_buf[0] == '#' || json_message_map_buf[0] == 10 || json_message_map_buf[0] == ';' || json_message_map_buf[0] == 32)
                {
                    continue;
                }


            JSON_Message_Map = (_JSON_Message_Map *) realloc(JSON_Message_Map, (counters->json_message_map+1) * sizeof(_JSON_Message_Map));


            if ( JSON_Message_Map == NULL )
                {
                    Sagan_Log(ERROR, "[%s, line %d] Failed to reallocate memory for _JSON_Message_Map. Abort!", __FILE__, __LINE__);
                }

            memset(&JSON_Message_Map[counters->json_message_map], 0, sizeof(struct _JSON_Message_Map));


            /* Set all values to NULL or 0 */

            JSON_Message_Map[counters->json_message_map].software[0] = '\0';
            JSON_Message_Map[counters->json_message_map].program[0] = '\0';
            JSON_Message_Map[counters->json_message_map].src_ip[0] = '\0';
            JSON_Message_Map[counters->json_message_map].dst_ip[0] = '\0';
            JSON_Message_Map[counters->json_message_map].src_port[0] = '\0';
            JSON_Message_Map[counters->json_message_map].dst_port[0] = '\0';
            JSON_Message_Map[counters->json_message_map].proto[0] = '\0';

            JSON_Message_Map[counters->json_message_map].md5[0] = '\0';
            JSON_Message_Map[counters->json_message_map].sha1[0] = '\0';
            JSON_Message_Map[counters->json_message_map].sha256[0] = '\0';
            JSON_Message_Map[counters->json_message_map].filename[0] = '\0';
            JSON_Message_Map[counters->json_message_map].hostname[0] = '\0';
            JSON_Message_Map[counters->json_message_map].url[0] = '\0';
            JSON_Message_Map[counters->json_message_map].ja3[0] = '\0';

            json_obj = json_tokener_parse(json_message_map_buf);

            if ( json_obj == NULL )
                {
                    Sagan_Log(ERROR, "[%s, line %d] JSON message map is incorrect at: \"%s\"", __FILE__, __LINE__, json_message_map_buf);
                    json_object_put(json_obj);
                    return;
                }

            if ( json_object_object_get_ex(json_obj, "software", &tmp))
                {

                    const char *software = json_object_get_string(tmp);

                    if ( software != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].software,  software, sizeof(JSON_Message_Map[counters->json_message_map].software));
                        }

                }

            if ( json_object_object_get_ex(json_obj, "program", &tmp))
                {

                    const char *program = json_object_get_string(tmp);

                    if ( program != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].program,  program, sizeof(JSON_Message_Map[counters->json_message_map].program));
                        }

                }

            /* Suricata event_type == program */

            if ( json_object_object_get_ex(json_obj, "event_type", &tmp))
                {

                    const char *event_type = json_object_get_string(tmp);

                    if ( event_type != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].program,  event_type, sizeof(JSON_Message_Map[counters->json_message_map].program));
                        }
                }

            /* Pull in "message" values.  It could be multiple valus for "message" */

            if ( json_object_object_get_ex(json_obj, "message", &tmp))
                {

                    data = (char*)json_object_get_string(tmp);

                    if ( data != NULL )
                        {

                            Remove_Spaces(data);

                            ptr2 = strtok_r(data, ",", &ptr1);

                            while ( ptr2 != NULL )
                                {

                                    strlcpy(JSON_Message_Map[counters->json_message_map].message[JSON_Message_Map[counters->json_message_map].message_count], ptr2, sizeof(JSON_Message_Map[counters->json_message_map].message[JSON_Message_Map[counters->json_message_map].message_count]));

                                    JSON_Message_Map[counters->json_message_map].message_count++;

                                    ptr2 = strtok_r(NULL, ",", &ptr1);
                                }
                        }

                }

            if ( json_object_object_get_ex(json_obj, "src_ip", &tmp))
                {

                    const char *src_ip = json_object_get_string(tmp);

                    if ( src_ip != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].src_ip,  src_ip, sizeof(JSON_Message_Map[counters->json_message_map].src_ip));
                        }
                }

            /* "dest_ip" is for Suricata compatibility */

            if ( json_object_object_get_ex(json_obj, "dst_ip", &tmp) || json_object_object_get_ex(json_obj, "dest_ip", &tmp) )
                {

                    const char *dst_ip = json_object_get_string(tmp);

                    if ( dst_ip != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].dst_ip,  dst_ip, sizeof(JSON_Message_Map[counters->json_message_map].dst_ip));
                        }
                }

            /* Suricata compatibility */

            /*
                        if ( json_object_object_get_ex(json_obj, "dest_ip", &tmp))
                            {

                                strlcpy(JSON_Message_Map[counters->json_message_map].dst_ip,  json_object_get_string(tmp), sizeof(JSON_Message_Map[counters->json_message_map].dst_ip));
                            }
            */

            if ( json_object_object_get_ex(json_obj, "src_port", &tmp))
                {

                    const char *src_port = json_object_get_string(tmp);

                    if ( src_port != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].src_port,  src_port, sizeof(JSON_Message_Map[counters->json_message_map].src_port));
                        }

                }

            if ( json_object_object_get_ex(json_obj, "dst_port", &tmp) || json_object_object_get_ex(json_obj, "dest_port", &tmp) )
                {

                    const char *dst_port = json_object_get_string(tmp);

                    if ( dst_port != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].dst_port,  dst_port, sizeof(JSON_Message_Map[counters->json_message_map].dst_port));
                        }

                }

            /* Suricata compatibility */
            /*
                        if ( json_object_object_get_ex(json_obj, "dest_port", &tmp))
                            {
                                strlcpy(JSON_Message_Map[counters->json_message_map].dst_port,  json_object_get_string(tmp), sizeof(JSON_Message_Map[counters->json_message_map].dst_port));
                            }
            */

            if ( json_object_object_get_ex(json_obj, "proto", &tmp))
                {

                    const char *proto = json_object_get_string(tmp);

                    if ( proto != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].proto, proto, sizeof(JSON_Message_Map[counters->json_message_map].proto));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "flow_id", &tmp))
                {
                    const char *flow_id = json_object_get_string(tmp);

                    if ( flow_id != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].flow_id,  flow_id, sizeof(JSON_Message_Map[counters->json_message_map].flow_id));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "md5", &tmp))
                {

                    const char *md5 = json_object_get_string(tmp);

                    if ( md5 != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].md5, md5, sizeof(JSON_Message_Map[counters->json_message_map].md5));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "sha1", &tmp))
                {

                    const char *sha1 = json_object_get_string(tmp);

                    if ( sha1 != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].sha1,  sha1, sizeof(JSON_Message_Map[counters->json_message_map].sha1));
                        }

                }

            if ( json_object_object_get_ex(json_obj, "sha256", &tmp))
                {

                    const char *sha256 = json_object_get_string(tmp);

                    if ( sha256 != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].sha256,  sha256, sizeof(JSON_Message_Map[counters->json_message_map].sha256));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "filename", &tmp))
                {

                    const char *filename = json_object_get_string(tmp);

                    if ( filename != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].filename,  filename, sizeof(JSON_Message_Map[counters->json_message_map].filename));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "hostname", &tmp))
                {

                    const char *hostname = json_object_get_string(tmp);

                    if ( hostname != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].hostname,  hostname,  sizeof(JSON_Message_Map[counters->json_message_map].hostname));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "url", &tmp))
                {

                    const char *url = json_object_get_string(tmp);

                    if ( url != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].url,  url, sizeof(JSON_Message_Map[counters->json_message_map].url));
                        }
                }

            if ( json_object_object_get_ex(json_obj, "ja3", &tmp))
                {

                    const char *ja3 = json_object_get_string(tmp);

                    if ( ja3 != NULL )
                        {
                            strlcpy(JSON_Message_Map[counters->json_message_map].ja3,  ja3, sizeof(JSON_Message_Map[counters->json_message_map].ja3));
                        }
                }


            counters->json_message_map++;

        }

    json_object_put(json_obj);

}

/************************************************************************
 * Parse_JSON_Message - Parses mesage (or program+message) for JSON data
 ************************************************************************/

void Parse_JSON_Message ( _Sagan_Proc_Syslog *SaganProcSyslog_LOCAL )
{

    struct _JSON_Message_Map_Found *JSON_Message_Map_Found = NULL;
    JSON_Message_Map_Found = malloc(sizeof(struct _JSON_Message_Map_Found) * JSON_MAX_NEST);

    if ( JSON_Message_Map_Found == NULL )
        {
            Sagan_Log(ERROR, "[%s, line %d] Failed to allocate memory for JSON_Message_Map_Found. Abort!", __FILE__, __LINE__);
        }

    memset(JSON_Message_Map_Found, 0, sizeof(_JSON_Message_Map_Found) * JSON_MAX_NEST);

    uint16_t i=0;
    uint16_t a=0;
    uint16_t b=0;

    uint32_t score=0;
    uint32_t prev_score=0;
    uint32_t pos=0;

    /* We start at 1 because the SaganProcSyslog_LOCAL->message is considered the
       first JSON string */

    uint16_t json_str_count=1;

    bool has_message;
    bool found = false;

    struct json_object *json_obj = NULL;
    struct json_object *json_obj2 = NULL;

    struct json_object *tmp = NULL;

    char json_str[JSON_MAX_NEST][JSON_MAX_SIZE];  // = { { 0 } };
    char tmp_message[MAX_SYSLOGMSG] = { 0 };

    strlcpy(json_str[0], SaganProcSyslog_LOCAL->syslog_message, sizeof(json_str[0]));
    json_obj = json_tokener_parse(SaganProcSyslog_LOCAL->syslog_message);

    /* If JSON parsing fails, it wasn't JSON after all */

    if ( json_obj == NULL )
        {

            if ( debug->debugmalformed )
                {
                    Sagan_Log(WARN, "[%s, line %d] Sagan Detected JSON but Libfastjson failed to decode it. The log line was: \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->syslog_message);
                }

            json_object_put(json_obj);
            free(JSON_Message_Map_Found);
            __atomic_add_fetch(&counters->malformed_json_mp_count, 1, __ATOMIC_SEQ_CST);
            return;
        }



    if ( debug->debugjson )
        {
            Sagan_Log(DEBUG, "Syslog Message: |%s|\n", SaganProcSyslog_LOCAL->syslog_message);
        }

    struct json_object_iterator it = json_object_iter_begin(json_obj);
    struct json_object_iterator itEnd = json_object_iter_end(json_obj);

    while (!json_object_iter_equal(&it, &itEnd))
        {

            const char *key = json_object_iter_peek_name(&it);
            struct json_object *const val = json_object_iter_peek_value(&it);

            const char *val_str = json_object_get_string(val);

            if ( debug->debugjson )
                {
                    Sagan_Log(DEBUG, "Key: \"%s\", Value: \"%s\"", key, val_str );

                }

            /* Is there nested JSON */

            if ( val_str != NULL && val_str[0] == '{' )
                {

                    /* Validate it before handing it to the parser to save CPU */

                    json_obj2 = json_tokener_parse(val_str);

                    if ( json_obj2 != NULL )
                        {


                            strlcpy(json_str[json_str_count], val_str, sizeof(json_str[json_str_count]));
                            json_str_count++;

                            struct json_object_iterator it2 = json_object_iter_begin(json_obj2);
                            struct json_object_iterator itEnd2 = json_object_iter_end(json_obj2);

                            /* Look for any second tier/third tier JSON */

                            while (!json_object_iter_equal(&it2, &itEnd2))
                                {

                                    const char *key2 = json_object_iter_peek_name(&it2);
                                    struct json_object *const val2 = json_object_iter_peek_value(&it2);

                                    const char *val_str2 = json_object_get_string(val2);

                                    if ( debug->debugjson )
                                        {
                                            Sagan_Log(DEBUG, "Key2: \"%s\", Value: \"%s\"", key2, val_str );

                                        }


                                    if ( val_str2[0] == '{' )
                                        {

                                            strlcpy(json_str[json_str_count], val_str2, sizeof(json_str[json_str_count]));
                                            json_str_count++;

                                        }

                                    json_object_iter_next(&it2);

                                }
                        } /* json_obj2 != NULL */

                    json_object_put(json_obj2);

                }


            json_object_iter_next(&it);

        }


    if ( debug->debugjson )
        {

            for ( a=0; a < json_str_count; a++ )
                {
                    Sagan_Log(DEBUG, "[%s, line %d] %d. JSON found: \"%s\"",  __FILE__, __LINE__, a, json_str[a]);

                }
        }


    /* Search message maps and see which one's match our syslog message best */

    for (i = 0; i < counters->json_message_map; i++ )
        {
            score = 0;

            for ( a = 0; a < json_str_count; a++ )
                {

                    struct json_object *json_obj = NULL;
                    json_obj = json_tokener_parse(json_str[a]);

                    if ( json_obj == NULL )
                        {
                            Sagan_Log(WARN, "[%s, line %d] Detected JSON Nest but function was incorrect. The log line was: \"%s\"", __FILE__, __LINE__, json_str[a]);
                            json_object_put(json_obj);
                            free(JSON_Message_Map_Found);
                            return;
                        }



                    if ( !strcmp(JSON_Message_Map[i].message[0], "%JSON%" ) )
                        {
                            strlcpy( JSON_Message_Map_Found[i].message, SaganProcSyslog_LOCAL->syslog_message, sizeof(JSON_Message_Map_Found[i].message) );
                            has_message = true;
                            score++;
                        }

                    if ( JSON_Message_Map[i].message_count > 1 )
                        {


                            for ( b=0; b < JSON_Message_Map[i].message_count; b++ )
                                {


                                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].message[b], &tmp))
                                        {

                                            const char *message = json_object_get_string(tmp);

                                            if ( message != NULL )
                                                {

                                                    snprintf(tmp_message, sizeof(tmp_message), "%s: %s ,", JSON_Message_Map[i].message[b], message);
                                                    strlcat(JSON_Message_Map_Found[i].message, tmp_message, sizeof(JSON_Message_Map_Found[i].message));

                                                    has_message=true;
                                                    score++;
                                                }

                                        }

                                }

                        }
                    else
                        {

                            if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].message[0], &tmp) )
                                {

                                    const char *message = json_object_get_string(tmp);

                                    if ( message != NULL )
                                        {

                                            snprintf(JSON_Message_Map_Found[i].message, sizeof(JSON_Message_Map_Found[i].message), "%s: %s", JSON_Message_Map[i].message[0], message);

                                        }

                                }


                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].program, &tmp))
                        {


                            const char *program = json_object_get_string(tmp);

                            if ( program != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].program, program, sizeof(JSON_Message_Map_Found[i].program));
                                    score++;
                                }

                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].src_ip, &tmp))
                        {
                            const char *src_ip = json_object_get_string(tmp);

                            if ( src_ip != NULL )
                                {

                                    strlcpy(JSON_Message_Map_Found[i].src_ip, src_ip, sizeof(JSON_Message_Map_Found[i].src_ip));
                                    score++;

                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].dst_ip, &tmp))
                        {

                            const char *dst_ip = json_object_get_string(tmp);

                            if ( dst_ip != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].dst_ip, dst_ip, sizeof(JSON_Message_Map_Found[i].dst_ip));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].src_port, &tmp))
                        {
                            const char *src_port = json_object_get_string(tmp);

                            if ( src_port != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].src_port, src_port, sizeof(JSON_Message_Map_Found[i].src_port));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].dst_port, &tmp))
                        {

                            const char *dst_port = json_object_get_string(tmp);

                            if ( dst_port != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].dst_port, dst_port, sizeof(JSON_Message_Map_Found[i].dst_port));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].proto, &tmp))
                        {


                            const char *proto = json_object_get_string(tmp);

                            if ( proto != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].proto, proto, sizeof(JSON_Message_Map_Found[i].proto));
                                    score++;
                                }
                        }


                    /* Suricata specific */

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].flow_id, &tmp))
                        {

                            const char *flow_id = json_object_get_string(tmp);

                            if ( flow_id != NULL )
                                {
                                    JSON_Message_Map_Found[i].flow_id = atol( flow_id );
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].md5, &tmp))
                        {

                            const char *md5 = json_object_get_string(tmp);

                            if ( md5 != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].md5, md5, sizeof(JSON_Message_Map_Found[i].md5));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].sha1, &tmp))
                        {

                            const char *sha1 = json_object_get_string(tmp);

                            if ( sha1 != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].sha1, sha1, sizeof(JSON_Message_Map_Found[i].sha1));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].sha256, &tmp))
                        {

                            const char *sha256 = json_object_get_string(tmp);

                            if ( sha256 != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].sha256, sha256, sizeof(JSON_Message_Map_Found[i].sha256));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].filename, &tmp))
                        {

                            const char *filename = json_object_get_string(tmp);

                            if ( filename != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].filename, filename, sizeof(JSON_Message_Map_Found[i].filename));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].hostname, &tmp))
                        {

                            const char *hostname = json_object_get_string(tmp);

                            if ( hostname != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].hostname, hostname, sizeof(JSON_Message_Map_Found[i].hostname));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].url, &tmp))
                        {

                            const char *url = json_object_get_string(tmp);

                            if ( url != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].url, url, sizeof(JSON_Message_Map_Found[i].url));
                                    score++;
                                }
                        }

                    if ( json_object_object_get_ex(json_obj, JSON_Message_Map[i].ja3, &tmp))
                        {

                            const char *ja3 = json_object_get_string(tmp);

                            if ( ja3 != NULL )
                                {
                                    strlcpy(JSON_Message_Map_Found[i].ja3, ja3, sizeof(JSON_Message_Map_Found[i].ja3));
                                    score++;
                                }
                        }


                    json_object_put(json_obj);

                }

            if ( score > prev_score && has_message == true )
                {

                    pos = i;
                    prev_score = score;
                    found = true;
                }

        }

    if ( debug->debugjson )
        {

            if ( found == true )
                {
                    Sagan_Log(DEBUG, "[%s, line %d] Best message mapping match is at postion %s (%d) (score of %d)", __FILE__, __LINE__, JSON_Message_Map[pos].software, pos, prev_score );
                }
            else
                {
                    Sagan_Log(DEBUG, "[%s, line %d] No JSON mappings found", __FILE__, __LINE__);
                }

        }

    /* We have to have a "message!" */

    if ( found == true )
        {

            __atomic_add_fetch(&counters->json_mp_count, 1, __ATOMIC_SEQ_CST);

            /* Put JSON values into place */

            /* If this is "message":"{value},{value},{value}", get rid of trailing , in the new "message */

            if ( JSON_Message_Map_Found[pos].message[ strlen(JSON_Message_Map_Found[pos].message) - 1 ] == ',' )
                {
                    JSON_Message_Map_Found[pos].message[ strlen(JSON_Message_Map_Found[pos].message) - 1 ] = '\0';
                }

            /* Copy our new message for the engine to use */

            strlcpy(SaganProcSyslog_LOCAL->syslog_message, JSON_Message_Map_Found[pos].message, sizeof(SaganProcSyslog_LOCAL->syslog_message));

            /* Adopt the "flow_id" */

            SaganProcSyslog_LOCAL->flow_id = JSON_Message_Map_Found[pos].flow_id;

            if ( JSON_Message_Map_Found[pos].md5[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->md5, JSON_Message_Map_Found[pos].md5, sizeof(SaganProcSyslog_LOCAL->md5));
                }

            if ( JSON_Message_Map_Found[pos].sha1[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->sha1, JSON_Message_Map_Found[pos].sha1, sizeof(SaganProcSyslog_LOCAL->sha1));
                }

            if ( JSON_Message_Map_Found[pos].sha256[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->sha256, JSON_Message_Map_Found[pos].sha256, sizeof(SaganProcSyslog_LOCAL->sha256));
                }

            if ( JSON_Message_Map_Found[pos].filename[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->filename, JSON_Message_Map_Found[pos].filename, sizeof(SaganProcSyslog_LOCAL->filename));
                }

            if ( JSON_Message_Map_Found[pos].hostname[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->hostname, JSON_Message_Map_Found[pos].hostname, sizeof(SaganProcSyslog_LOCAL->hostname));
                }

            if ( JSON_Message_Map_Found[pos].url[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->url, JSON_Message_Map_Found[pos].url, sizeof(SaganProcSyslog_LOCAL->url));
                }


            if ( JSON_Message_Map_Found[pos].src_ip[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->src_ip, JSON_Message_Map_Found[pos].src_ip, sizeof(SaganProcSyslog_LOCAL->src_ip));
                }

            if ( JSON_Message_Map_Found[pos].dst_ip[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->dst_ip, JSON_Message_Map_Found[pos].dst_ip, sizeof(SaganProcSyslog_LOCAL->dst_ip));
                }

            if ( JSON_Message_Map_Found[pos].src_port[0] != '\0' )
                {
                    SaganProcSyslog_LOCAL->src_port = atoi(JSON_Message_Map_Found[pos].src_port);
                }

            if ( JSON_Message_Map_Found[pos].dst_port[0] != '\0' )
                {
                    SaganProcSyslog_LOCAL->dst_port = atoi(JSON_Message_Map_Found[pos].dst_port);
                }

            if ( JSON_Message_Map_Found[pos].ja3[0] != '\0' )
                {
                    strlcpy(SaganProcSyslog_LOCAL->ja3, JSON_Message_Map_Found[pos].ja3, sizeof(SaganProcSyslog_LOCAL->ja3));
                }

            if ( JSON_Message_Map_Found[pos].proto[0] != '\0' )
                {

                    if ( !strcasecmp( JSON_Message_Map_Found[pos].proto, "tcp" ) || !strcasecmp( JSON_Message_Map_Found[pos].proto, "TCP" ) )
                        {
                            SaganProcSyslog_LOCAL->proto = 6;
                        }

                    else if ( !strcasecmp( JSON_Message_Map_Found[pos].proto, "udp" ) || !strcasecmp( JSON_Message_Map_Found[pos].proto, "UDP" ) )
                        {
                            SaganProcSyslog_LOCAL->proto = 17;
                        }

                    else if ( !strcasecmp( JSON_Message_Map_Found[pos].proto, "icmp" ) || !strcasecmp( JSON_Message_Map_Found[pos].proto, "ICMP" ) )
                        {
                            SaganProcSyslog_LOCAL->proto = 1;
                        }

                }

            /* Don't override syslog program if no program is present */

            if ( JSON_Message_Map_Found[pos].program[0] != '\0' )
                {

                    strlcpy(SaganProcSyslog_LOCAL->syslog_program, JSON_Message_Map_Found[pos].program, sizeof(SaganProcSyslog_LOCAL->syslog_program));
                    Remove_Spaces(SaganProcSyslog_LOCAL->syslog_program);

                }


            if ( debug->debugjson )
                {
                    Sagan_Log(DEBUG, "[%s, line %d] New data extracted from JSON:", __FILE__, __LINE__);
                    Sagan_Log(DEBUG, "[%s, line %d] -------------------------------------------------------", __FILE__, __LINE__);
                    Sagan_Log(DEBUG, "[%s, line %d] Message: \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->syslog_message );
                    Sagan_Log(DEBUG, "[%s, line %d] Program: \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->syslog_program );
                    Sagan_Log(DEBUG, "[%s, line %d] src_ip : \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->src_ip );
                    Sagan_Log(DEBUG, "[%s, line %d] dst_ip : \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->dst_ip );
                    Sagan_Log(DEBUG, "[%s, line %d] src_port : \"%d\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->src_port );
                    Sagan_Log(DEBUG, "[%s, line %d] dst_port : \"%d\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->dst_port );
                    Sagan_Log(DEBUG, "[%s, line %d] proto : \"%d\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->proto );
                    Sagan_Log(DEBUG, "[%s, line %d] ja3: \"%s\"", __FILE__, __LINE__, SaganProcSyslog_LOCAL->ja3 );
                }


        }

    free(JSON_Message_Map_Found);
    json_object_put(json_obj);

}

#endif
