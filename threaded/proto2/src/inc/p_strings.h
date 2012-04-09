extern void prim_split(PRIM_PROTOTYPE);
extern void prim_rsplit(PRIM_PROTOTYPE);
extern void prim_ctoi(PRIM_PROTOTYPE);
extern void prim_itoc(PRIM_PROTOTYPE);
extern void prim_stod(PRIM_PROTOTYPE);
extern void prim_dtos(PRIM_PROTOTYPE);
extern void prim_midstr(PRIM_PROTOTYPE);
extern void prim_numberp(PRIM_PROTOTYPE);
extern void prim_stringcmp(PRIM_PROTOTYPE);
extern void prim_strcmp(PRIM_PROTOTYPE);
extern void prim_strncmp(PRIM_PROTOTYPE);
extern void prim_strcut (PRIM_PROTOTYPE);
extern void prim_strlen(PRIM_PROTOTYPE);
extern void prim_strcat(PRIM_PROTOTYPE);
extern void prim_atoi(PRIM_PROTOTYPE);
extern void prim_ansi_notify(PRIM_PROTOTYPE);
extern void prim_ansi_notify_exclude(PRIM_PROTOTYPE);
extern void prim_notify(PRIM_PROTOTYPE);
extern void prim_notify_descriptor(PRIM_PROTOTYPE);
extern void prim_ansi_notify_descriptor(PRIM_PROTOTYPE);
extern void prim_notify_exclude(PRIM_PROTOTYPE);
extern void prim_intostr(PRIM_PROTOTYPE);
extern void prim_explode(PRIM_PROTOTYPE);
extern void prim_subst(PRIM_PROTOTYPE);
extern void prim_instr(PRIM_PROTOTYPE);
extern void prim_rinstr(PRIM_PROTOTYPE);
extern void prim_pronoun_sub(PRIM_PROTOTYPE);
extern void prim_toupper(PRIM_PROTOTYPE);
extern void prim_tolower(PRIM_PROTOTYPE);
extern void prim_unparseobj(PRIM_PROTOTYPE);
extern void prim_smatch(PRIM_PROTOTYPE);
extern void prim_striplead(PRIM_PROTOTYPE);
extern void prim_striptail(PRIM_PROTOTYPE);
extern void prim_stringpfx(PRIM_PROTOTYPE);
extern void prim_strencrypt(PRIM_PROTOTYPE);
extern void prim_strdecrypt(PRIM_PROTOTYPE);
extern void prim_tokensplit(PRIM_PROTOTYPE);
extern void prim_fmtstring(PRIM_PROTOTYPE);
extern void prim_parse_ansi(PRIM_PROTOTYPE);
extern void prim_parse_neon(PRIM_PROTOTYPE);
extern void prim_unparse_ansi(PRIM_PROTOTYPE);
extern void prim_escape_ansi(PRIM_PROTOTYPE);
extern void prim_ansi_strlen(PRIM_PROTOTYPE);
extern void prim_ansi_strcut(PRIM_PROTOTYPE);
extern void prim_ansi_strip(PRIM_PROTOTYPE);
extern void prim_ansi_midstr(PRIM_PROTOTYPE);
extern void prim_array_fmtstrings(PRIM_PROTOTYPE);
extern void prim_textattr(PRIM_PROTOTYPE);
extern void prim_flag_2char(PRIM_PROTOTYPE);
extern void prim_power_2char(PRIM_PROTOTYPE);
extern void prim_notify_descriptor_char(PRIM_PROTOTYPE);
extern void prim_ansi_unparseobj(PRIM_PROTOTYPE);
extern void prim_ansi_name(PRIM_PROTOTYPE);
extern void prim_base64encode(PRIM_PROTOTYPE);
extern void prim_base64decode(PRIM_PROTOTYPE);
#ifdef UTF8_SUPPORT
extern void prim_wcharlen(PRIM_PROTOTYPE);
extern void prim_wcharlen_slice(PRIM_PROTOTYPE);
#endif

#define PRIMLIST_STRINGS		{ "NUMBER?",				LM1, 1, prim_numberp },						\
								{ "STRINGCMP",				LM1, 2, prim_stringcmp },					\
								{ "STRCMP",					LM1, 2, prim_strcmp },						\
								{ "STRNCMP",				LM1, 3, prim_strncmp },						\
								{ "STRCUT",					LM1, 2, prim_strcut },						\
								{ "STRLEN",					LM1, 1, prim_strlen },						\
								{ "STRCAT",					LM1, 2, prim_strcat },						\
								{ "ATOI",					LM1, 1, prim_atoi },						\
								{ "ANSI_NOTIFY",			LM1, 2, prim_ansi_notify },					\
								{ "NOTIFY",					LM1, 2, prim_notify },						\
								{ "NOTIFY_DESCRIPTOR",		LMAGE, 2, prim_notify_descriptor },			\
								{ "ANSI_NOTIFY_DESCRIPTOR",	LMAGE, 2, prim_ansi_notify_descriptor },	\
								{ "NOTIFY_EXCLUDE",			LM1, 2, prim_notify_exclude },				\
								{ "INTOSTR",				LM1, 1, prim_intostr },						\
								{ "EXPLODE",				LM1, 2, prim_explode },						\
								{ "SUBST",					LM1, 3, prim_subst },						\
								{ "INSTR",					LM1, 2, prim_instr },						\
								{ "RINSTR",					LM1, 2, prim_rinstr },						\
								{ "PRONOUN_SUB",			LM1, 2, prim_pronoun_sub },					\
								{ "TOUPPER",				LM1, 1, prim_toupper },						\
								{ "TOLOWER",				LM1, 1, prim_tolower },						\
								{ "UNPARSEOBJ",				LM1, 1, prim_unparseobj },					\
								{ "SMATCH",					LM1, 2, prim_smatch },						\
								{ "STRIPLEAD",				LM1, 1, prim_striplead },					\
								{ "STRIPTAIL",				LM1, 1, prim_striptail },					\
								{ "STRINGPFX",				LM1, 2, prim_stringpfx },					\
								{ "STRENCRYPT",				LM1, 2, prim_strencrypt },					\
								{ "STRDECRYPT",				LM1, 2, prim_strdecrypt },					\
								{ "MIDSTR",					LM1, 3, prim_midstr },						\
								{ "CTOI",					LM1, 1, prim_ctoi },						\
								{ "ITOC",					LM1, 1, prim_itoc },						\
								{ "STOD",					LM1, 1, prim_stod },						\
								{ "SPLIT",					LM1, 2, prim_split },						\
								{ "RSPLIT",					LM1, 2, prim_rsplit },						\
								{ "ANSI_NOTIFY_EXCLUDE",	LM1, 2, prim_ansi_notify_exclude },			\
								{ "DTOS",					LM1, 1, prim_dtos },						\
								{ "TOKENSPLIT",				LM1, 3, prim_tokensplit },					\
								{ "FMTSTRING",				LM1, 1, prim_fmtstring },					\
								{ "PARSE_ANSI",				LM1, 2, prim_parse_ansi },					\
								{ "UNPARSE_ANSI",			LM1, 2, prim_unparse_ansi },				\
								{ "ESCAPE_ANSI",			LM1, 2, prim_escape_ansi },					\
								{ "ANSI_STRLEN",			LM1, 1, prim_ansi_strlen },					\
								{ "ANSI_STRCUT",			LM1, 2, prim_ansi_strcut },					\
								{ "ANSI_STRIP",				LM1, 1, prim_ansi_strip },					\
								{ "ANSI_MIDSTR",			LM1, 3, prim_ansi_midstr },					\
								{ "TEXTATTR",				LM1, 2, prim_textattr },					\
								{ "PARSE_NEON",				LM1, 3, prim_parse_neon },					\
								{ "DESCRNOTIFY",			LMAGE, 2, prim_notify_descriptor },			\
								{ "FLAG_2CHAR",				LM1, 1, prim_flag_2char },					\
								{ "POWER_2CHAR",			LM1, 1, prim_power_2char },					\
								{ "NOTIFY_DESCRIPTOR_CHAR",	LMAGE, 2, prim_notify_descriptor_char },	\
								{ "ARRAY_FMTSTRINGS",		LM1, 2, prim_array_fmtstrings },			\
								{ "ANSI_UNPARSEOBJ",		LM1, 1, prim_ansi_unparseobj },				\
								{ "ANSI_NAME",				LM1, 1, prim_ansi_name },					\
								{ "BASE64ENCODE",			LM1, 1, prim_base64encode },				\
								{ "BASE64DECODE",			LM1, 1, prim_base64decode }

#define PRIMS_STRINGS_CNT 60

#ifdef UTF8_SUPPORT

#define PRIMLIST_STRINGS_UTF8	{ "WCHARLEN",				LM1, 1, prim_wcharlen },					\
								{ "WCHARLEN_SLICE",			LM1, 2, prim_wcharlen_slice }

#define PRIMS_STRINGS_UTF8_CNT 2

#endif