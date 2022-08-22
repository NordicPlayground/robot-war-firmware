#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/mesh/msg.h>
#include <bluetooth/mesh/models.h>


#include "model_handler.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(model_handler);

struct model_handlers *app_handlers;

/* SIG models */
static void attention_on(struct bt_mesh_model *model)
{
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    printk("attention_off()\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);




void app_handle_rx(void *data, size_t len, uint32_t opcode, uint16_t model_id, uint16_t addr)
{
    if (app_handlers) {
        if (app_handlers->rx) {
            app_handlers->rx((uint8_t *)data, len, opcode, model_id, addr);
        }
    }
}

void handle_robot_id(struct bt_mesh_robot_cli *cli, struct bt_mesh_id_status id, struct bt_mesh_msg_ctx *ctx) 
{    
	LOG_HEXDUMP_INF(id.id, CONFIG_BT_MESH_ID_LEN, "Id detected:");
    app_handle_rx(&id, sizeof(struct bt_mesh_id_status), BT_MESH_ID_OP_STATUS, ID_CLI_MODEL_ID, ctx->addr);
}

void handle_robot_movement_configured(struct bt_mesh_robot_cli *cli, struct bt_mesh_msg_ctx *ctx) 
{    
	LOG_INF("movement configured on addr %x", ctx->addr);
    uint8_t ack = 0;
    app_handle_rx(&ack, sizeof(uint8_t), BT_MESH_MOVEMENT_OP_MOVEMENT_ACK, MOVEMENT_CLI_MODEL_ID, ctx->addr);
}

void handle_robot_telemetry_reported(struct bt_mesh_robot_cli *cli, struct bt_mesh_telemetry_report report, struct bt_mesh_msg_ctx *ctx) 
{    
	LOG_INF("telemetry reported from addr %x", ctx->addr);
    app_handle_rx(&report, sizeof(struct bt_mesh_telemetry_report), BT_MESH_TELEMETRY_OP_TELEMETRY_REPORT, TELEMETRY_CLI_MODEL_ID, ctx->addr);
}

/* Vendor models */
static const struct bt_mesh_robot_cli_handlers robot_cb = {
	.id = handle_robot_id,
    .movement_configured = handle_robot_movement_configured,
    .telemetry_reported = handle_robot_telemetry_reported,
};

struct bt_mesh_robot_cli robot = BT_MESH_ROBOT_CLI_INIT(&robot_cb);

struct bt_mesh_light_rgb_cli light_rgb;

/* Composition */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(
		0,
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_CFG_SRV,
            BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)
        ),
        BT_MESH_MODEL_LIST( 
            BT_MESH_MODEL_ROBOT_CLI(&robot),
            BT_MESH_MODEL_LIGHT_RGB_CLI(&light_rgb)
        )
    )
};

static struct bt_mesh_comp comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(struct model_handlers *handlers)
{
    app_handlers = handlers;
    return &comp;
}

int mesh_tx(uint8_t *data, uint8_t len, uint32_t type, uint16_t model_id, uint16_t addr)
{
    switch (model_id)
	{
	case MOVEMENT_CLI_MODEL_ID:
	{
		if (type == BT_MESH_MOVEMENT_OP_MOVEMENT_SET) {
            struct bt_mesh_movement_set set;

            struct bt_mesh_msg_ctx ctx = {
                .addr = addr,
                .app_idx = robot.model->keys[0],
                .send_ttl = BT_MESH_TTL_DEFAULT
            };

            uint8_t *set_ptr = (uint8_t*)&set;
	        memcpy(set_ptr, data, sizeof(uint32_t));

            set_ptr = set_ptr + sizeof(uint32_t);
            data = data +  sizeof(uint32_t);
            memcpy(set_ptr, data, sizeof(uint32_t));

            set_ptr = set_ptr + sizeof(uint32_t);
            data = data +  sizeof(uint32_t);
            memcpy(set_ptr, data, sizeof(uint8_t));

            
            bt_mesh_robot_cli_movement_set(&robot, &ctx, set);
		}
        else if (type == BT_MESH_MOVEMENT_OP_READY_SET) {

            struct bt_mesh_msg_ctx ctx = {
                .addr = 0xFFFF,
                .app_idx = robot.model->keys[0],
                .send_ttl = BT_MESH_TTL_DEFAULT
            };

            bt_mesh_robot_cli_ready_set(&robot, &ctx);
		}
	} break;
	case LIGHT_RGB_CLI_MODEL_ID:
    {
        if (type == BT_MESH_LIGHT_RGB_OP_RGB_SET) {
            struct bt_mesh_light_rgb_set set;

            struct bt_mesh_msg_ctx ctx = {
                .addr = addr,
                .app_idx = light_rgb.model->keys[0],
                .send_ttl = BT_MESH_TTL_DEFAULT
            };

            uint8_t *set_ptr = (uint8_t*)&set;
	        memcpy(set_ptr, data, sizeof(uint16_t));

            set_ptr = set_ptr + sizeof(uint16_t);
            data = data +  sizeof(uint16_t);
            memcpy(set_ptr, data, sizeof(uint8_t));

            set_ptr = set_ptr + sizeof(uint8_t);
            data = data +  sizeof(uint8_t);
            memcpy(set_ptr, data, sizeof(uint8_t));
            
            set_ptr = set_ptr + sizeof(uint8_t);
            data = data +  sizeof(uint8_t);
            memcpy(set_ptr, data, sizeof(uint8_t));

            bt_mesh_light_rgb_cli_rgb_set(&light_rgb, &ctx, set);
		}
    } break;
   
	default:
		break;
	}
    return 0;
}