#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <json.h>
#include <ws.h>

typedef struct chat_client
{
    ws_cli_conn_t *client;
    char username[33];
} * chat_client;

int client_size;
chat_client *clients;

void onopen(ws_cli_conn_t *client)
{
    // nothing needed
}

void onclose(ws_cli_conn_t *client)
{
    for (int i = 0; i < client_size; i++)
        if (clients[i]->client == client)
        {
            // broadcast
            char buffer[47];
            strcpy(buffer, clients[i]->username);
            strcat(buffer, " disconnected.");
            struct json_object *response = json_object_new_object();
            json_object_object_add(response, "type", json_object_new_string("server_message"));
            json_object_object_add(response, "message", json_object_new_string(buffer));

            for (int i = 0; i < client_size; i++)
                ws_sendframe_txt(clients[i]->client, json_object_to_json_string(response));

            // remove client
            printf("%sINFO:%s %s disconnected.\n", "\033[0;36m", "\033[0m", clients[i]->username);
            ws_close_client(client);
            free(clients[i]);
            client_size--;
            memcpy((clients + (i * sizeof(void))), (clients + ((i + 1) * sizeof(void))), (client_size - i) * sizeof(void *));
            clients = realloc(clients, client_size * sizeof(void *));
            break;
        }
}

void onmessage(ws_cli_conn_t *client, const unsigned char *msg, uint64_t size, int type)
{
    bool logged_in = false;
    char *username;
    struct json_object *message = json_tokener_parse((const char *)msg);

    // check auth status
    for (int i = 0; i < client_size; i++)
        if (clients[i]->client == client)
        {
            logged_in = true;
            username = clients[i]->username;
            break;
        }

    if (logged_in)
    {
        // send message to all
        chat_client client_info;
        char *text = (char *)json_object_get_string(json_object_object_get(message, "message"));

        for (int i = 0; i < client_size; i++)
            if (clients[i]->client == client)
                client_info = clients[i];

        printf("%s%s:%s %s\n", "\033[0;35m", client_info->username, "\033[0m", text);

        struct json_object *response = json_object_new_object();
        json_object_object_add(response, "type", json_object_new_string("message"));
        json_object_object_add(response, "username", json_object_new_string(client_info->username));
        json_object_object_add(response, "message", json_object_object_get(message, "message"));

        for (int i = 0; i < client_size; i++)
            ws_sendframe_txt(clients[i]->client, json_object_to_json_string(response));
    }
    else
    {
        // login
        char *desired_name = (char *)json_object_get_string(json_object_object_get(message, "username"));

        // check if wanted name is too long or too short
        if (strlen(desired_name) > 32 || strlen(desired_name) < 3)
        {
            struct json_object *response = json_object_new_object();
            json_object_object_add(response, "error", json_object_new_string("Username is an invalid length."));
            json_object_object_add(response, "success", json_object_new_boolean(false));
            ws_sendframe_txt(client, json_object_to_json_string(response));
            ws_close_client(client);
            return;
        }

        // check if wanted name exists
        for (int i = 0; i < client_size; i++)
            if (!strcasecmp(clients[i]->username, desired_name))
            {
                struct json_object *response = json_object_new_object();
                json_object_object_add(response, "error", json_object_new_string("Username already exists."));
                json_object_object_add(response, "success", json_object_new_boolean(false));
                ws_sendframe_txt(client, json_object_to_json_string(response));
                ws_close_client(client);
                return;
            }

        // register
        chat_client client_info = calloc(1, sizeof(struct chat_client));
        client_info->client = client;
        strcpy(client_info->username, desired_name);

        // add to array
        client_size++;
        clients = realloc(clients, client_size * sizeof(struct chat_client));
        clients[client_size - 1] = client_info;

        struct json_object *response = json_object_new_object();
        json_object_object_add(response, "success", json_object_new_boolean(true));
        ws_sendframe_txt(client, json_object_to_json_string(response));

        char buffer[44];
        strcpy(buffer, desired_name);
        strcat(buffer, " connected.");
        struct json_object *broadcast = json_object_new_object();
        json_object_object_add(broadcast, "type", json_object_new_string("server_message"));
        json_object_object_add(broadcast, "message", json_object_new_string(buffer));

        for (int i = 0; i < client_size; i++)
            ws_sendframe_txt(clients[i]->client, json_object_to_json_string(broadcast));

        printf("%sINFO:%s %s connected.\n", "\033[0;36m", "\033[0m", desired_name);
    }
}

int main(void)
{
    // create socket
    struct ws_events events;
    events.onopen = &onopen;
    events.onclose = &onclose;
    events.onmessage = &onmessage;
    printf("%sINFO:%s ", "\033[0;36m", "\033[0m");
    ws_socket(&events, 8080, 1, 1000);

    char buffer[16];

    // allows quit at any point with :q || quit
    while (true)
    {
        scanf("%s", buffer);

        if (strlen(buffer) < 1)
            continue;

        char *arg0 = strtok(buffer, " ");
        arg0 = strtok(arg0, "\n");

        if (!strcasecmp(arg0, ":q") || !strcasecmp(arg0, "quit"))
        {
            printf("%sINFO:%s Shutting down server.\n", "\033[0;36m", "\033[0m");

            for (int i = 0; i < client_size; i++)
            {
                struct json_object *response = json_object_new_object();
                json_object_object_add(response, "type", json_object_new_string("disconnect"));
                ws_sendframe_txt(clients[i]->client, json_object_to_json_string(response));
                onclose(clients[i]->client);
            }

            free(clients);

            break;
        }
        else
            printf("%sERROR:%s Unrecognized command.\n", "\033[0;31m", "\033[0m");
    }

    return (0);
}
