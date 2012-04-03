#ifndef _P_MCP_H
#define _P_MCP_H

#ifdef MCP_SUPPORT

extern void prim_mcp_register(PRIM_PROTOTYPE);
extern void prim_mcp_register_event(PRIM_PROTOTYPE);
extern void prim_mcp_bind(PRIM_PROTOTYPE);
extern void prim_mcp_supports(PRIM_PROTOTYPE);
extern void prim_mcp_send(PRIM_PROTOTYPE);

extern void prim_gui_available(PRIM_PROTOTYPE);
extern void prim_gui_dlog_create(PRIM_PROTOTYPE);
extern void prim_gui_dlog_show(PRIM_PROTOTYPE);
extern void prim_gui_dlog_close(PRIM_PROTOTYPE);
extern void prim_gui_value_set(PRIM_PROTOTYPE);
extern void prim_gui_values_get(PRIM_PROTOTYPE);
extern void prim_gui_ctrl_create(PRIM_PROTOTYPE);
extern void prim_gui_ctrl_command(PRIM_PROTOTYPE);
extern void prim_gui_value_get(PRIM_PROTOTYPE);


#define PRIMS_MCP_FUNCS prim_mcp_register, prim_mcp_bind, prim_mcp_supports,     \
		prim_mcp_send, prim_gui_available, prim_gui_dlog_show,               \
		prim_gui_dlog_close, prim_gui_values_get, prim_gui_value_set,        \
		prim_gui_ctrl_create, prim_gui_dlog_create, prim_mcp_register_event, \
            prim_gui_ctrl_command, prim_gui_value_get

#define PRIMS_MCP_NAMES "MCP_REGISTER", "MCP_BIND", "MCP_SUPPORTS",     \
		"MCP_SEND", "GUI_AVAILABLE", "GUI_DLOG_SHOW",               \
		"GUI_DLOG_CLOSE", "GUI_VALUES_GET", "GUI_VALUE_SET",        \
		"GUI_CTRL_CREATE", "GUI_DLOG_CREATE", "MCP_REGISTER_EVENT", \
            "GUI_CTRL_COMMAND", "GUI_VALUE_GET"
            
#define PRIMLIST_MCP    { "MCP_REGISTER",       LM1, 3, prim_mcp_register },       \
                        { "MCP_BIND",           LM1, 3, prim_mcp_bind },           \
                        { "MCP_SUPPORTS",       LM1, 2, prim_mcp_supports },       \
                        { "MCP_SEND",           LM1, 4, prim_mcp_send },           \
                        { "GUI_AVAILABLE",      LM1, 1, prim_gui_available },      \
                        { "GUI_DLOG_SHOW",      LM1, 1, prim_gui_dlog_show },      \
                        { "GUI_DLOG_CLOSE",     LM1, 1, prim_gui_dlog_close },     \
                        { "GUI_VALUES_GET",     LM1, 1, prim_gui_values_get },     \
                        { "GUI_VALUE_SET",      LM1, 3, prim_gui_value_set },      \
                        { "GUI_CTRL_CREATE",    LM1, 4, prim_gui_ctrl_create },    \
                        { "GUI_DLOG_CREATE",    LM1, 4, prim_gui_dlog_create },    \
                        { "MCP_REGISTER_EVENT", LM1, 3, prim_mcp_register_event }, \
                        { "GUI_CTRL_COMMAND",   LM1, 4, prim_gui_ctrl_command },   \
                        { "GUI_VALUE_GET",      LM1, 2, prim_gui_value_get }
                        


#define PRIMS_MCP_CNT 14

#else

#define PRIMS_MCP_CNT 0

#endif

#endif /* _P_MCP_H */


