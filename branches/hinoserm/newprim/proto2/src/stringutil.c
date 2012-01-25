#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"
#include "interp.h"

/* String utilities */

#include "externs.h"

#define DOWNCASE(x) (tolower(x))

/* Hash table of color names->numeric color index lookup */
static hash_tab color_list[COLOR_HASH_SIZE];

int init_color_hash()
{
    add_hash_int("XT/BLACK", 16, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/STATOS", 17, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/NAVYBLUE", 18, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DARKBLUE", 19, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DARKBLUE2", 20, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BLUE", 21, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CAMARONE", 22, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BLUESTONE", 23, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ORIENT", 24, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ENDEAVOUR", 25, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCIENCEBLUE", 26, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BLUERIBBON", 27, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/JAPANESELAUREL", 28, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DEEPSEA", 29, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TEAL", 30, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DEEPCERULEAN", 31, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LOCHMARA", 32, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AZURERADIANCE", 33, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/JAPANESELAUREL2", 34, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/JADE", 35, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PERSIANGREEN", 36, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BONDIBLUE", 37, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CERULEAN", 38, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AZURERADIANCE2", 39, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GREEN", 40, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MALACHITE", 41, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CARIBBEANGREEN", 42, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CARIBBEANGREEN2", 43, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROBINSEGGBLUE", 44, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AQUA", 45, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GREEN2", 46, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SPRINGGREEN", 47, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SPRINGGREEN2", 48, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SPRINGGREEN3", 49, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BRIGHTTURQUOISE", 50, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CYAN", 51, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROSEWOOD", 52, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/POMPADOUR", 53, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PIGMENTINDIGO", 54, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/VERDUNGREEN", 58, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCORPION", 59, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/COMET", 60, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCAMPI", 61, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/INDIGO", 62, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CORNFLOWERBLUE", 63, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIMEADE", 64, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GLADEGREEN", 65, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/JUNIPER", 66, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HIPPIEBLUE", 67, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HAVELOCKBLUE", 68, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CORNFLOWERBLUE2", 69, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIMEADE2", 70, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FERN", 71, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SILVERTREE", 72, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TRADEWIND", 73, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SHAKESPEARE", 74, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MALIBU", 75, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BRIGHTGREEN", 76, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PASTELGREEN", 77, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PASTELGREEN2", 78, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DOWNY", 79, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/VIKING", 80, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MALIBU2", 81, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BRIGHTGREEN2", 82, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCREAMINGREEN", 83, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCREAMINGREEN2", 84, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AQUAMARINE", 85, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AQUAMARINE2", 86, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AQUAMARINE3", 87, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MAROON", 88, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FRESHEGGPLANT", 89, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FRESHEGGPLANT2", 90, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PURPLE", 91, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ELECTRICVIOLET", 92, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BROWN", 94, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/COPPERROSE", 95, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/STRIKEMASTER", 96, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DELUGE", 97, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MEDIUMPURPLE", 98, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HELIOTROPE", 99, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OLIVE", 100, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CLAYCREEK", 101, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GRAY", 102, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/WILDBLUEYONDER", 103, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CHETWODEBLUE", 104, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MALIBU3", 105, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIMEADE3", 106, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CHELSEACUCUMBER", 107, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BAYLEAF", 108, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GULFSTREAM", 109, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/POLOBLUE", 110, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MALIBU4", 111, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PISTACHIO", 112, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PASTELGREEN3", 113, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FEIJOA", 114, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/VISTABLUE", 115, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BERMUDA", 116, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ANAKIWA", 117, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CHARTREUSE", 118, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SCREAMINGREEN3", 119, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MINTGREEN", 120, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MINTGREEN2", 121, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AQUAMARINE4", 122, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ANAKIWA2", 123, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BRIGHTRED", 124, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FLIRT", 125, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FLIRT2", 126, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FLIRT3", 127, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ELECTRICVIOLET2", 128, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROSEOFSHARON", 130, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MATRIX", 131, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TAPESTRY", 132, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FUCHSIAPINK", 133, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MEDIUMPURPLE2", 134, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HELIOTROPE2", 135, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PIRATEGOLD", 136, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MUESLI", 137, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PHARLAP", 138, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BOUQUET", 139, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LAVENDER", 140, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HELIOTROPE3", 141, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BUDDHAGOLD", 142, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OLIVEGREEN", 143, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HILLARY", 144, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SILVERCHALICE", 145, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/WISTFUL", 146, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MELROSE", 147, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/RIOGRANDE", 148, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CONIFER", 149, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FEIJOA2", 150, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PIXIEGREEN", 151, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/JUNGLEMIST", 152, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ANAKIWA3", 153, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIME", 154, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GREENYELLOW", 155, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MINTGREEN3", 156, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MINTGREEN4", 157, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/AEROBLUE", 158, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FRENCHPASS", 159, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GUARDSMANRED", 160, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/RAZZMATAZZ", 161, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOLLYWOODCERISE", 162, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOLLYWOODCERISE2", 163, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PURPLEPIZZAZZ", 164, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ELECTRICVIOLET3", 165, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TENN", 166, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROMAN", 167, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CRANBERRY", 168, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOPBUSH", 169, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ORCHID", 170, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HELIOTROPE4", 171, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MANGOTANGO", 172, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/COPPERFIELD", 173, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MYPINK", 174, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CANCAN", 175, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIGHTORCHID", 176, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HELIOTROPE5", 177, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CORN", 178, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TACHA", 179, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TAN", 180, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CLAMSHELL", 181, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/THISTLE", 182, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MAUVE", 183, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CORN2", 184, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TACHA2", 185, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DECO", 186, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GREENMIST", 187, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ALTO", 188, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FOG", 189, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CHARTREUSEYELLOW", 190, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CANARY", 191, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HONEYSUCKLE", 192, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/REEF", 193, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SNOWYMINT", 194, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OYSTERBAY", 195, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/RED", 196, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROSE", 197, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ROSE2", 198, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOLLYWOODCERISE3", 199, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PURPLEPIZZAZZ2", 200, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MAGENTA", 201, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BLAZEORANGE", 202, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BITTERSWEET", 203, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/WILDWATERMELON", 204, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOTPINK", 205, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HOTPINK2", 206, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PINKFLAMINGO", 207, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/FLUSHORANGE", 208, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SALMON", 209, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/VIVIDTANGERINE", 210, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PINKSALMON", 211, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LAVENDERROSE", 212, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BLUSHPINK", 213, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/YELLOWSEA", 214, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TEXASROSE", 215, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/HITPINK", 216, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SUNDOWN", 217, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/COTTONCANDY", 218, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LAVENDERROSE2", 219, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GOLD", 220, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DANDELION", 221, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GRANDIS", 222, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CARAMEL", 223, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/COSMOS", 224, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PINKLACE", 225, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/YELLOW", 226, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LASERLEMON", 227, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DOLLY", 228, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PORTAFINO", 229, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CUMULUS", 230, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/WHITE", 231, color_list, COLOR_HASH_SIZE);    
    add_hash_int("XT/BLACKHOLE", 232, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OUTERSPACE", 233, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OBSIDIAN", 234, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TAR", 235, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CATBLACK", 236, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/CHARCOAL", 237, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/RAVEN", 238, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SLATE", 239, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/TIMBERWOLF", 240, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DIMGRAY", 241, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DARKGRAY", 242, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ULTRAGRAY", 243, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/LIGHTGRAY", 244, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/STEEL", 245, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/GRANITE", 246, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/INDUSTRIAL", 247, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/BATTLESHIP", 248, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/SILVER", 249, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DUSKY", 250, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/ASHEN", 251, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/MOONROCK", 252, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/OYSTER", 253, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/PEARL", 254, color_list, COLOR_HASH_SIZE);
    add_hash_int("XT/DOVE", 255, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLACK", 316, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/STATOS", 317, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/NAVYBLUE", 318, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DARKBLUE", 319, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DARKBLUE2", 320, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLUE", 321, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CAMARONE", 322, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLUESTONE", 323, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ORIENT", 324, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ENDEAVOUR", 325, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCIENCEBLUE", 326, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLUERIBBON", 327, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/JAPANESELAUREL", 328, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DEEPSEA", 329, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TEAL", 330, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DEEPCERULEAN", 331, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LOCHMARA", 332, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AZURERADIANCE", 333, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/JAPANESELAUREL2", 334, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/JADE", 335, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PERSIANGREEN", 336, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BONDIBLUE", 337, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CERULEAN", 338, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AZURERADIANCE2", 339, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GREEN", 340, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MALACHITE", 341, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CARIBBEANGREEN", 342, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CARIBBEANGREEN2", 343, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROBINSEGGBLUE", 344, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AQUA", 345, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GREEN2", 346, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SPRINGGREEN", 347, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SPRINGGREEN2", 348, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SPRINGGREEN3", 349, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BRIGHTTURQUOISE", 350, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CYAN", 351, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROSEWOOD", 352, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/POMPADOUR", 353, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PIGMENTINDIGO", 354, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/VERDUNGREEN", 358, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCORPION", 359, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/COMET", 360, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCAMPI", 361, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/INDIGO", 362, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CORNFLOWERBLUE", 363, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIMEADE", 364, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GLADEGREEN", 365, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/JUNIPER", 366, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HIPPIEBLUE", 367, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HAVELOCKBLUE", 368, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CORNFLOWERBLUE2", 369, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIMEADE2", 370, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FERN", 371, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SILVERTREE", 372, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TRADEWIND", 373, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SHAKESPEARE", 374, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MALIBU", 375, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BRIGHTGREEN", 376, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PASTELGREEN", 377, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PASTELGREEN2", 378, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DOWNY", 379, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/VIKING", 380, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MALIBU2", 381, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BRIGHTGREEN2", 382, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCREAMINGREEN", 383, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCREAMINGREEN2", 384, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AQUAMARINE", 385, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AQUAMARINE2", 386, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AQUAMARINE3", 387, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MAROON", 388, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FRESHEGGPLANT", 389, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FRESHEGGPLANT2", 390, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PURPLE", 391, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ELECTRICVIOLET", 392, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BROWN", 394, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/COPPERROSE", 395, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/STRIKEMASTER", 396, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DELUGE", 397, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MEDIUMPURPLE", 398, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HELIOTROPE", 399, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OLIVE", 400, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CLAYCREEK", 401, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GRAY", 402, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/WILDBLUEYONDER", 403, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CHETWODEBLUE", 404, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MALIBU3", 405, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIMEADE3", 406, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CHELSEACUCUMBER", 407, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BAYLEAF", 408, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GULFSTREAM", 409, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/POLOBLUE", 410, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MALIBU4", 411, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PISTACHIO", 412, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PASTELGREEN3", 413, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FEIJOA", 414, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/VISTABLUE", 415, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BERMUDA", 416, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ANAKIWA", 417, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CHARTREUSE", 418, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SCREAMINGREEN3", 419, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MINTGREEN", 420, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MINTGREEN2", 421, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AQUAMARINE4", 422, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ANAKIWA2", 423, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BRIGHTRED", 424, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FLIRT", 425, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FLIRT2", 426, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FLIRT3", 427, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ELECTRICVIOLET2", 428, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROSEOFSHARON", 430, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MATRIX", 431, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TAPESTRY", 432, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FUCHSIAPINK", 433, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MEDIUMPURPLE2", 434, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HELIOTROPE2", 435, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PIRATEGOLD", 436, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MUESLI", 437, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PHARLAP", 438, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BOUQUET", 439, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LAVENDER", 440, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HELIOTROPE3", 441, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BUDDHAGOLD", 442, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OLIVEGREEN", 443, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HILLARY", 444, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SILVERCHALICE", 445, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/WISTFUL", 446, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MELROSE", 447, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/RIOGRANDE", 448, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CONIFER", 449, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FEIJOA2", 450, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PIXIEGREEN", 451, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/JUNGLEMIST", 452, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ANAKIWA3", 453, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIME", 454, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GREENYELLOW", 455, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MINTGREEN3", 456, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MINTGREEN4", 457, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/AEROBLUE", 458, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FRENCHPASS", 459, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GUARDSMANRED", 460, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/RAZZMATAZZ", 461, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOLLYWOODCERISE", 462, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOLLYWOODCERISE2", 463, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PURPLEPIZZAZZ", 464, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ELECTRICVIOLET3", 465, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TENN", 466, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROMAN", 467, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CRANBERRY", 468, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOPBUSH", 469, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ORCHID", 470, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HELIOTROPE4", 471, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MANGOTANGO", 472, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/COPPERFIELD", 473, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MYPINK", 474, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CANCAN", 475, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIGHTORCHID", 476, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HELIOTROPE5", 477, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CORN", 478, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TACHA", 479, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TAN", 480, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CLAMSHELL", 481, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/THISTLE", 482, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MAUVE", 483, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CORN2", 484, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TACHA2", 485, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DECO", 486, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GREENMIST", 487, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ALTO", 488, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FOG", 489, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CHARTREUSEYELLOW", 490, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CANARY", 491, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HONEYSUCKLE", 492, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/REEF", 493, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SNOWYMINT", 494, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OYSTERBAY", 495, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/RED", 496, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROSE", 497, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ROSE2", 498, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOLLYWOODCERISE3", 499, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PURPLEPIZZAZZ2", 500, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MAGENTA", 501, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLAZEORANGE", 502, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BITTERSWEET", 503, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/WILDWATERMELON", 504, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOTPINK", 505, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HOTPINK2", 506, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PINKFLAMINGO", 507, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/FLUSHORANGE", 508, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SALMON", 509, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/VIVIDTANGERINE", 510, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PINKSALMON", 511, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LAVENDERROSE", 512, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLUSHPINK", 513, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/YELLOWSEA", 514, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TEXASROSE", 515, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/HITPINK", 516, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SUNDOWN", 517, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/COTTONCANDY", 518, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LAVENDERROSE2", 519, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GOLD", 520, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DANDELION", 521, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GRANDIS", 522, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CARAMEL", 523, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/COSMOS", 524, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PINKLACE", 525, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/YELLOW", 526, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LASERLEMON", 527, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DOLLY", 528, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PORTAFINO", 529, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CUMULUS", 530, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/WHITE", 531, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BLACKHOLE", 532, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OUTERSPACE", 533, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OBSIDIAN", 534, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TAR", 535, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CATBLACK", 536, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/CHARCOAL", 537, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/RAVEN", 538, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SLATE", 539, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/TIMBERWOLF", 540, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DIMGRAY", 541, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DARKGRAY", 542, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ULTRAGRAY", 543, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/LIGHTGRAY", 544, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/STEEL", 545, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/GRANITE", 546, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/INDUSTRIAL", 547, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/BATTLESHIP", 548, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/SILVER", 549, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DUSKY", 550, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/ASHEN", 551, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/MOONROCK", 552, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/OYSTER", 553, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/PEARL", 554, color_list, COLOR_HASH_SIZE);
    add_hash_int("XTB/DOVE", 555, color_list, COLOR_HASH_SIZE);

    return 1;
}

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int
alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    const char *s1 = t1;

    while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;

    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
        u1 = s1;
        u2 = s2;
        n1 = n2 = 0;            /* clear number values */
        cnt1 = cnt2 = 0;

        /* back up before zeros */
        if (s1 > t1 && *s2 == '0')
            s1--, s2--;         /* curr chars are diff */
        while (s1 > t1 && *s1 == '0')
            s1--, s2--;         /* prev chars are same */
        if (!isdigit(*s1))
            s1++, s2++;

        /* calculate number values */
        while (isdigit(*s1))
            cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
        while (isdigit(*s2))
            cnt2++, n2 = (n2 * 10) + (*s2++ - '0');

        /* if more digits than int can handle... */
        if (cnt1 > 8 || cnt2 > 8) {
            if (cnt1 == cnt2)
                return (*u1 - *u2); /* cmp chars if mag same */
            return (cnt1 - cnt2); /* compare magnitudes */
        }

        /* if number not same, return count difference */
        if (n1 && n2 && n1 != n2)
            return (n1 - n2);

        /* else, return difference of characters */
        return (*u1 - *u2);
    }
    /* if characters not digits, and not the same, return difference */
    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int
string_compare(const char *s1, const char *s2)
{
    while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;

    return (DOWNCASE(*s1) - DOWNCASE(*s2));

    //return (strcasecmp(s1, s2));
}

const char *
exit_prefix(const char *string, const char *prefix)
{
    const char *p;
    const char *s = string;

    while (*s) {
        p = prefix;
        string = s;
        while (*s && *p && DOWNCASE(*s) == DOWNCASE(*p)) {
            s++;
            p++;
        }
        while (*s && isspace(*s))
            s++;
        if (!*p && (!*s || *s == EXIT_DELIMITER)) {
            return string;
        }
        while (*s && (*s != EXIT_DELIMITER))
            s++;
        if (*s)
            s++;
        while (*s && isspace(*s))
            s++;
    }
    return 0;
}

int
string_prefix(const char *string, const char *prefix)
{
    while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
        string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
const char *
string_match(const char *src, const char *sub)
{
    if (*sub != '\0') {
        while (*src) {
            if (string_prefix(src, sub))
                return src;
            /* else scan to beginning of next word */
            while (*src && isalnum(*src))
                src++;
            while (*src && !isalnum(*src))
                src++;
        }
    }
    return 0;
}

#define MUSH_TAB "    "

char *
mushformat_substitute(const char *str)
{
    char c;
    //char d;
    char prn[3];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;

    prn[0] = '%';
    prn[2] = '\0';

    strcpy(orig, str);
    str = orig;

    result = buf;
    while (*str) {
        if (*str == '%') {
            *result = '\0';
            prn[1] = c = *(++str);
            if (!c) {
                *(result++) = '%';
                continue;
            } else if (c == '%') {
                *(result++) = '%';
                str++;
            } else {
                c = (isupper(c)) ? c : toupper(c);
                switch (c) {
                    case 'T': /* tab */
                        strcatn(result, sizeof(buf) - (result - buf),
                                MUSH_TAB);
                        break;
                    case 'B': /* single whitespace, kinda pointless */
                        strcatn(result, sizeof(buf) - (result - buf),
                                " ");
                        break;
                    case 'R': /* carriage return */
                        if (*--result == '\r') {
                            result++;
                            /* necessary to make \r\r work */
                            strcatn(result, sizeof(buf) - (result - buf),
                                    " \r");
                        } else {
                            result++;
                            strcatn(result, sizeof(buf) - (result - buf),
                                    "\r");
                        }
                        break;
                    default:
                        /* if strict_mush_escapes is tuned off, don't gobble
                           unhandled % escapes */
                        if ( !tp_strict_mush_escapes )
                            *(result++) = '%';
                        *result = *str;
                        result[1] = '\0';
                        break;
                }
                result += strlen(result);
                str++;
                if ((result - buf) > (BUFFER_LEN - 2)) {
                    buf[BUFFER_LEN - 1] = '\0';
                    return buf;
                }
            }
        } else {
            if ((result - buf) > (BUFFER_LEN - 2)) {
                buf[BUFFER_LEN - 1] = '\0';
                return buf;
            }
            *result++ = *str++;
        }
    }
    *result = '\0';
    return buf;
}

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/hirs/its, His/Hers/Hirs/Its)
 * %s/%S for subjective pronouns (he/she/sie/it, He/She/Sie/It)
 * %o/%O for objective pronouns (him/her/hir/it, Him/Her/Hir/It)
 * %p/%P for possessive pronouns (his/her/hir/its, His/Her/Hir/Its)
 * %r/%R for reflexive pronouns (himself/herself/hirself/itself,
 *                                Himself/Herself/Hirself/Itself)
 * %n    for the player's name.
 */
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char c;
    char d;
    char prn[3];
    char globprop[128];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;
    const char *sexstr;
    const char *self_sub;       /* self substitution code */
    const char *temp_sub;
    dbref mywhere = player;
    int sex;

    static const char *subjective[4] = { "", "it", "she", "he" };
    static const char *possessive[4] = { "", "its", "her", "his" };
    static const char *objective[4] = { "", "it", "her", "him" };
    static const char *reflexive[4] = { "", "itself", "herself", "himself" };
    static const char *absolute[4] = { "", "its", "hers", "his" };

    prn[0] = '%';
    prn[2] = '\0';

    strcpy(orig, str);
    str = orig;

    sex = genderof(descr, player);
    sexstr = get_property_class(player, tp_sex_prop);
    /*
       if (sexstr) {
       sexstr = do_parse_mesg(descr, player, player, sexstr, "(Lock)", sexbuf, MPI_ISLOCK);
       }
     */
    while (sexstr && isspace(*sexstr))
        sexstr++;
    if (!sexstr || !*sexstr) {
        sexstr = "_default";
    }
    result = buf;
    while (*str) {
        if (*str == '%') {
            *result = '\0';
            prn[1] = c = *(++str);
            if (!c) {
                *(result++) = '%';
                continue;
            } else if (c == '%') {
                *(result++) = '%';
                str++;
            } else {
                mywhere = player;
                d = (isupper(c)) ? c : toupper(c);

                snprintf(globprop, sizeof(globprop), "_pronouns/%.64s/%s",
                         sexstr, prn);
                if (d == 'A' || d == 'S' || d == 'O' || d == 'P' || d == 'R'
                    || d == 'N') {
                    self_sub = get_property_class(mywhere, prn);
                } else {
                    self_sub = envpropstr(&mywhere, prn);
                }
                if (!self_sub) {
                    self_sub = get_property_class(player, globprop);
                }
                if (!self_sub) {
                    self_sub = get_property_class(0, globprop);
                }
                if (!self_sub && (sex == GENDER_UNASSIGNED)) {
                    snprintf(globprop, sizeof(globprop),
                             "_pronouns/_default/%s", prn);

                    if (!(self_sub = get_property_class(player, globprop)))
                        self_sub = get_property_class(0, globprop);
                }
                if (self_sub) {
                    temp_sub = NULL;
                    if (self_sub[0] == '%' && toupper(self_sub[1]) == 'N') {
                        temp_sub = self_sub;
                        self_sub = PNAME(player);
                    }
                    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
                        return buf;
                    strcatn(result, sizeof(buf) - (result - buf), self_sub);
                    if (isupper(prn[1]) && islower(*result))
                        *result = toupper(*result);
                    result += strlen(result);
                    str++;
                    if (temp_sub) {
                        if (((result - buf) + strlen(temp_sub + 2)) >
                            (BUFFER_LEN - 2))
                            return buf;
                        strcatn(result, sizeof(buf) - (result - buf),
                                temp_sub + 2);
                        if (isupper(temp_sub[1]) && islower(*result))
                            *result = toupper(*result);
                        result += strlen(result);
                        str++;
                    }
                } else if (sex == GENDER_UNASSIGNED) {
                    switch (c) {
                        case 'n':
                        case 'N':
                        case 'o':
                        case 'O':
                        case 's':
                        case 'S':
                        case 'r':
                        case 'R':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
                            break;
                        case 'a':
                        case 'A':
                        case 'p':
                        case 'P':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
                            strcatn(result, sizeof(buf) - (result - buf), "'s");
                            break;
                        default:
                            result[0] = *str;
                            result[1] = 0;
                            break;
                    }
                    str++;
                    result += strlen(result);
                    if ((result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                } else {
                    switch (c) {
                        case 'a':
                        case 'A':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    absolute[sex]);
                            break;
                        case 's':
                        case 'S':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    subjective[sex]);
                            break;
                        case 'p':
                        case 'P':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    possessive[sex]);
                            break;
                        case 'o':
                        case 'O':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    objective[sex]);
                            break;
                        case 'r':
                        case 'R':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    reflexive[sex]);
                            break;
                        case 'n':
                        case 'N':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
                            break;
                        default:
                            *result = *str;
                            result[1] = '\0';
                            break;
                    }
                    if (isupper(c) && islower(*result)) {
                        *result = toupper(*result);
                    }
                    result += strlen(result);
                    str++;
                    if ((result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                }
            }
        } else {
            if ((result - buf) > (BUFFER_LEN - 2)) {
                buf[BUFFER_LEN - 1] = '\0';
                return buf;
            }
            *result++ = *str++;
        }
    }
    *result = '\0';
    return buf;
}


/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/its, His/Hers/Its)
 * %s/%S for subjective pronouns (he/she/it, He/She/It)
 * %o/%O for objective pronouns (him/her/it, Him/Her/It)
 * %p/%P for possessive pronouns (his/her/its, His/Her/Its)
 * %r/%R for reflexive pronouns (himself/herself/itself,
 *                                Himself/Herself/Itself)
 * %n    for the player's name.
 
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char c;
    char d;
    char prn[3];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;
    const char *self_sub;   
    dbref mywhere = player;
    int sex;

    static const char *subjective[4] = { "", "it", "she", "he" };
    static const char *possessive[4] = { "", "its", "her", "his" };
    static const char *objective[4] = { "", "it", "her", "him" };
    static const char *reflexive[4] = { "", "itself", "herself", "himself" };
    static const char *absolute[4] = { "", "its", "hers", "his" };

    prn[0] = '%';
    prn[2] = '\0';

    strcpy(orig, str);
    str = orig;

    sex = genderof(descr, player);
    result = buf;
    while (*str) {
        if (*str == '%') {
            *result = '\0';
            prn[1] = c = *(++str);
            if (c == '%') {
                *(result++) = '%';
                str++;
            } else {
                mywhere = player;
                d = (isupper(c)) ? c : toupper(c);

                if (d == 'A' || d == 'S' || d == 'O' ||
                    d == 'P' || d == 'R' || d == 'N') {
                    self_sub = get_property_class(mywhere, prn);
                } else {
                    self_sub = envpropstr(&mywhere, prn);
                }

                if (self_sub) {
                    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
                        return buf;
                    strcat(result, self_sub);
                    if (isupper(prn[1]) && islower(*result))
                        *result = toupper(*result);
                    result += strlen(result);
                    str++;
                } else if (sex == GENDER_UNASSIGNED) {
                    switch (c) {
                        case 'n':
                        case 'N':
                        case 'o':
                        case 'O':
                        case 's':
                        case 'S':
                        case 'r':
                        case 'R':
                            strcat(result, PNAME(player));
                            break;
                        case 'a':
                        case 'A':
                        case 'p':
                        case 'P':
                            strcat(result, PNAME(player));
                            strcat(result, "'s");
                            break;
                        default:
                            result[0] = *str;
                            result[1] = 0;
                            break;
                    }
                    str++;
                    result += strlen(result);
                    if ((result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                } else {
                    switch (c) {
                        case 'a':
                        case 'A':
                            strcat(result, absolute[sex]);
                            break;
                        case 's':
                        case 'S':
                            strcat(result, subjective[sex]);
                            break;
                        case 'p':
                        case 'P':
                            strcat(result, possessive[sex]);
                            break;
                        case 'o':
                        case 'O':
                            strcat(result, objective[sex]);
                            break;
                        case 'r':
                        case 'R':
                            strcat(result, reflexive[sex]);
                            break;
                        case 'n':
                        case 'N':
                            strcat(result, PNAME(player));
                            break;
                        default:
                            *result = *str;
                            result[1] = '\0';
                            break;
                    }
                    if (isupper(c) && islower(*result)) {
                        *result = toupper(*result);
                    }
                    result += strlen(result);
                    str++;
                    if ((result - buf) > (BUFFER_LEN - 2)) {
                        buf[BUFFER_LEN - 1] = '\0';
                        return buf;
                    }
                }
            }
        } else {
            if ((result - buf) > (BUFFER_LEN - 2)) {
                buf[BUFFER_LEN - 1] = '\0';
                return buf;
            }
            *result++ = *str++;
        }
    }
    *result = '\0';
    return buf;
}
*/
#ifndef MALLOC_PROFILING

char *
alloc_string(const char *string)
{
    char *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
        return 0;

    if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
        fprintf(stderr, "PANIC: alloc_string() Out of Memory.\n");
        abort();
    }
    strcpy(s, string);
    return s;
}

/* Allocate a shared_string struct. alloc_prog_string(x) is a macro for
 * alloc_prog_string_exact(x,-2,-2). While using this function directly is more
 * efficient when you know your string length already, it's also dangerous if
 * you introduce off-by-one errors. As a general rule, stick to using
 * alloc_prog_string unless you're passing in low-level I/O or something that
 * is similarly specialized, and verify your math in a C debugger before commit.
 *
 * wclength is directly assigned. -2 means unknown length, -1 is a UTF-8 coding
 * convetion indicates that the string contains invalid multi-byte characters.
 * Use -2 if you don't know what a "wide character" is and it'll work just fine.
 * We do not calculate wclength automatically for the user; this is done on
 * demand inside of the "wclength" function when the stored value is -2.
 *
 * length is automatically caulcuated with strlen if the passed in value is <0,
 * but -2 is preferred because it matches the meaning of wclength. Use -2 unless
 * you're absolutely sure that your string match is accurate. (or better yet,
 * stick to using the alloc_prog_string(x) macro)
 *
 * Direct hate mail at brevantes.
 */
struct shared_string *
alloc_prog_string_exact(const char *s, int length, int wclength)
{
    struct shared_string *ss;

    if (s == NULL || *s == '\0' || length == 0)
        return (NULL);

    if (length < 0) {
        length = strlen(s);
    }
    if ((ss = (struct shared_string *)
         malloc(sizeof(struct shared_string) + length)) == NULL) {
        fprintf(stderr, "PANIC: alloc_prog_string() Out of Memory.\n");
        abort();
    }
    ss->links = 1;
    ss->length = length;
#ifdef UTF8_SUPPORT
    ss->wclength = wclength;
#endif
    bcopy(s, ss->data, ss->length + 1);
    return (ss);
}

#endif

#if !defined(MALLOC_PROFILING)
char *
string_dup(const char *s)
{
    char *p;

    p = (char *) malloc(1 + strlen(s));
    if (p)
        (void) strcpy(p, s);
    return (p);
}
#endif

char *
intostr(char *buf, int i)
{
    sprintf(buf, "%d", i);
    return (buf);
}


/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 96

static char enarr[256];
static int charset_count[] = { 96, 0 };
static int initialized_crypt = 0;

void
init_crypt(void)
{
    int i;

    for (i = 0; i <= 255; i++)
        enarr[i] = (char) i;
    for (i = 'A'; i <= 'M'; i++)
        enarr[i] = (char) enarr[i] + 13;
    for (i = 'N'; i <= 'Z'; i++)
        enarr[i] = (char) enarr[i] - 13;
    enarr['\r'] = 127;
    enarr[127] = '\r';
    initialized_crypt = 1;
}


const char *
strencrypt(const char *data, const char *key)
{
    static char linebuf[BUFFER_LEN];
    char buf[BUFFER_LEN + 8];
    const char *cp;
    unsigned char *ptr;
    unsigned char *ups;
    const unsigned char *upt;
    int linelen;
    int count;
    int seed, seed2, seed3;
    int limit = BUFFER_LEN;
    int result;

    if (!initialized_crypt)
        init_crypt();

    seed = 0;
    for (cp = key; *cp; cp++)
        seed = ((((*cp) ^ seed) + 170) % 192);

    seed2 = 0;
    for (cp = data; *cp; cp++)
        seed2 = ((((*cp) ^ seed2) + 21) & 0xff);
    seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

    count = seed + 11;
    for (upt = (const unsigned char *) data, ups = (unsigned char *) buf, cp =
         key; *upt; upt++) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
        seed2 = ((seed2 + 1) & 0x3f);
        if (!*(++cp))
            cp = key;
        result = (enarr[*upt] - (32 - (CHARCOUNT - 96))) + count + seed;
        *ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
        count = (((*upt) ^ count) + seed) & 0xff;
        ups++;
    }
    *ups++ = '\0';

    ptr = (unsigned char *) linebuf;

    linelen = strlen(data);
    *ptr++ = (' ' + 1);
    *ptr++ = (' ' + seed3);
    limit--;
    limit--;

    for (cp = buf; cp < &buf[linelen]; cp++) {
        limit--;
        if (limit < 0)
            break;
        *ptr++ = *cp;
    }
    *ptr++ = '\0';
    return linebuf;
}



const char *
strdecrypt(const char *data, const char *key)
{
    char linebuf[BUFFER_LEN];
    static char buf[BUFFER_LEN];
    const char *cp;
    unsigned char *ups;
    const unsigned char *upt;
    int outlen;
    int count;
    int seed, seed2;
    int result;
    int chrcnt;

    if (!initialized_crypt)
        init_crypt();

    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 1) {
        return "";
    }

    chrcnt = charset_count[(data[0] - ' ') - 1];
    seed2 = (data[1] - ' ');

    strcpy(linebuf, data + 2);

    seed = 0;
    for (cp = key; *cp; cp++)
        seed = (((*cp) ^ seed) + 170) % 192;

    count = seed + 11;
    outlen = strlen(linebuf);
    upt = (const unsigned char *) linebuf;
    ups = (unsigned char *) buf;
    cp = key;
    while ((const char *) upt < &linebuf[outlen]) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
        if (!*(++cp))
            cp = key;
        seed2 = ((seed2 + 1) & 0x3f);

        result = (enarr[*upt] - (32 - (chrcnt - 96))) - (count + seed);
        while (result < 0)
            result += chrcnt;
        *ups = enarr[result + (32 - (chrcnt - 96))];

        count = (((*ups) ^ count) + seed) & 0xff;
        ups++;
        upt++;
    }
    *ups++ = '\0';

    return buf;
}

/* This function is where the custom color support in Proto comes from. 
 * See documentation for details on how to use it. Players can set
 * personal prefs, global defaults are set on #0.
 */
const char *
color_lookup(dbref player, const char *color, const char *defcolor,
             int intrecurse, char *color_buffer)
{
    const char *tempcolor;
    char buf[BUFFER_LEN];
    int index = 0;
    hash_data *hd = NULL; 
    char temp_buffer[8];
    int search_more = 1; /* Set to false to block unnecessary searches */
    color_buffer[0] = '\0';
    temp_buffer[0] = '\0';

    if ((!color) || (!*color))
        return defcolor;
    if (player != NOTHING && OkObj(player)) {
        if (!strcasecmp("SUCC", color) || !strcasecmp("CSUCC", color)) {
            tempcolor = GETMESG(player, "_/COLORS/SUCC");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/SUCC");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/SUCC");
            if (!tempcolor) 
                tempcolor = CCSUCC;
            color = tempcolor;
        } else if (!strcasecmp("FAIL", color) || !strcasecmp("CFAIL", color)) {
            tempcolor = GETMESG(player, "_/COLORS/FAIL");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/FAIL");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/FAIL");
            if (!tempcolor) 
                tempcolor = CCFAIL;
            color = tempcolor;
        } else if (!strcasecmp("INFO", color) || !strcasecmp("CINFO", color)) {
            tempcolor = GETMESG(player, "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = CCINFO;
            color = tempcolor;
        } else if (!strcasecmp("NOTE", color) || !strcasecmp("CNOTE", color)) {
            tempcolor = GETMESG(player, "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = CCNOTE;
            color = tempcolor;
        } else if (!strcasecmp("MOVE", color) || !strcasecmp("CMOVE", color)) {
            tempcolor = GETMESG(player, "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = CCMOVE;
            color = tempcolor;
        } else {
            strcpy(buf, "_/COLORS/");
            strcat(buf, color);
            tempcolor = GETMESG(player, buf);
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), buf);
            if (!tempcolor)
                tempcolor = GETMESG(0, buf);
            if (tempcolor)
                color = tempcolor;
            else
                search_more = 0; 
        }

        if (intrecurse < 5 && search_more) {
            (void) intrecurse++;
            return color_lookup(player, color, defcolor, intrecurse, 
                                color_buffer );
        }
    }                           /* End of player != NOTHING check. Too lazy to indent all that. */

    if (!strcasecmp("NORMAL", color)) {
        return ANSINORMAL;
    } else if (!strcasecmp("BOLD", color) || !strcasecmp("BRIGHT", color)) {
        return ANSIBOLD;
    } else if (!strcasecmp("DIM", color) || !strcasecmp("HALFBRIGHT", color)) {
        return ANSIDIM;
    } else if (!strcasecmp("ITALIC", color) || !strcasecmp("ITALICS", color)) {
        return ANSIITALIC;
    } else if (!strcasecmp("UNDERLINE", color) || !strcasecmp("UNDERSCORE", color)) {
        return ANSIUNDERLINE;
    } else if (!strcasecmp("FLASH", color) || !strcasecmp("BLINK", color)) {
        return ANSIFLASH;
    } else if (!strcasecmp("FLASH2", color) || !strcasecmp("BLINK2", color)) {
        return ANSIFLASH2;
    } else if (!strcasecmp("INVERT", color) || !strcasecmp("REVERSE", color)) {
        return ANSIINVERT;
    } else if (!strcasecmp("INVISIBLE", color) || !strcasecmp("HIDDEN", color)) {
        return ANSIINVISIBLE;
    } else if (!strcasecmp("BLACK", color)) {
        return ANSIBLACK;
    } else if (!strcasecmp("CRIMSON", color)) {
        return ANSICRIMSON;
    } else if (!strcasecmp("FOREST", color)) {
        return ANSIFOREST;
    } else if (!strcasecmp("BROWN", color)) {
        return ANSIBROWN;
    } else if (!strcasecmp("NAVY", color)) {
        return ANSINAVY;
    } else if (!strcasecmp("VIOLET", color)) {
        return ANSIVIOLET;
    } else if (!strcasecmp("AQUA", color)) {
        return ANSIAQUA;
    } else if (!strcasecmp("GRAY", color)) {
        return ANSIGRAY;
    } else if (!strcasecmp("GLOOM", color)) {
        return ANSIGLOOM;
    } else if (!strcasecmp("RED", color)) {
        return ANSIRED;
    } else if (!strcasecmp("GREEN", color)) {
        return ANSIGREEN;
    } else if (!strcasecmp("YELLOW", color)) {
        return ANSIYELLOW;
    } else if (!strcasecmp("BLUE", color)) {
        return ANSIBLUE;
    } else if (!strcasecmp("PURPLE", color)) {
        return ANSIPURPLE;
    } else if (!strcasecmp("CYAN", color)) {
        return ANSICYAN;
    } else if (!strcasecmp("WHITE", color)) {
        return ANSIWHITE;
    } else if (!strcasecmp("CBLACK", color)) {
        return ANSICBLACK;
    } else if (!strcasecmp("CRED", color)) {
        return ANSICRED;
    } else if (!strcasecmp("CGREEN", color)) {
        return ANSICGREEN;
    } else if (!strcasecmp("CYELLOW", color)) {
        return ANSICYELLOW;
    } else if (!strcasecmp("CBLUE", color)) {
        return ANSICBLUE;
    } else if (!strcasecmp("CPURPLE", color)) {
        return ANSICPURPLE;
    } else if (!strcasecmp("CCYAN", color)) {
        return ANSICCYAN;
    } else if (!strcasecmp("CWHITE", color)) {
        return ANSICWHITE;
    } else if (!strcasecmp("BBLACK", color)) {
        return ANSIBBLACK;
    } else if (!strcasecmp("BRED", color)) {
        return ANSIBRED;
    } else if (!strcasecmp("BGREEN", color)) {
        return ANSIBGREEN;
    } else if (!strcasecmp("BBROWN", color)) {
        return ANSIBBROWN;
    } else if (!strcasecmp("BBLUE", color)) {
        return ANSIBBLUE;
    } else if (!strcasecmp("BPURPLE", color)) {
        return ANSIBPURPLE;
    } else if (!strcasecmp("BCYAN", color)) {
        return ANSIBCYAN;
    } else if (!strcasecmp("BGRAY", color)) {
        return ANSIBGRAY;
    } else if (!strcasecmp("HBLACK", color)) {
        return ANSIHBLACK;
    } else if (!strcasecmp("HRED", color)) {
        return ANSIHRED;
    } else if (!strcasecmp("HGREEN", color)) {
        return ANSIHGREEN;
    } else if (!strcasecmp("HYELLOW", color) || !strcasecmp("HBROWN", color)) {
        return ANSIHYELLOW;
    } else if (!strcasecmp("HBLUE", color)) {
        return ANSIHBLUE;
    } else if (!strcasecmp("HPURPLE", color)) {
        return ANSIHPURPLE;
    } else if (!strcasecmp("HCYAN", color)) {
        return ANSIHCYAN;
    } else if (!strcasecmp("HWHITE", color) || !strcasecmp("HGRAY", color)) {
        return ANSIHWHITE;
    } else if (!strcasecmp("STRIKE", color)) {
        return ANSI_STRIKE;
    } else if ( (hd = find_hash(color, color_list, COLOR_HASH_SIZE)) ){
       /* Color found in color lookup table */
        sprintf( temp_buffer, "%d", hd->ival);
        if (intrecurse < 5) {
            intrecurse++;
            return color_lookup(player, temp_buffer, defcolor, intrecurse,
                                color_buffer);
        }
    } else if ( number(color) || (!strncasecmp("B",color,1) && number(color+1)) ) {
        /* we have received a 256 color index */
        if (!strncasecmp("B",color,1) && number(color+1))
            index = atoi(color+1)+300;
        else 
            index = atoi(color);

        if ( index >= 0 && index < 256 ) {
            /* Index is 0 - 255,  forground */
            sprintf(color_buffer, "\033[38;5;%dm", index);
            return ANSI_256;
        } else if ( index >= 300 && index < 556 ) {
            /* Index is 300 - 555, i.e., background (0-255) */
            sprintf(color_buffer, "\033[48;5;%dm", index - 300);
            return ANSI_256;
        } else if ( index == 256 ) {
            /* Special case */
            return ANSI_256_RESET;
        } 
    }

    return defcolor;
}

/* parse_ansi: Converts Neon ANSI tags into standard ANSI for
 * output. I.e, ^RED^ -> \[[1;30m
 */
char *
parse_ansi(dbref player, char *buf, const char *from, const char *defcolor)
{
    char *to, *color, cbuf[BUFFER_LEN + 2];
    char *color_ptr = NULL;
    char color_buffer[64];
    const char *ansi;

    to = buf;
    while (*from) {
        if (*from == '^') {
            from++;
            color = cbuf;
            while (*from && *from != '^')
                *(color++) = (*(from++));
            *color = '\0';
            if (*from)
                from++;
            if (*cbuf) {
                if ((ansi = color_lookup(player, cbuf, defcolor, 1, 
                      color_buffer ))) {
                    if (!strcmp(ansi, ANSI_256)) {
                        /* Copy 256 color from buffer */
                        color_ptr = color_buffer;
                        while (*color_ptr) {
                            *(to++) = (*(color_ptr++));
                        } 
                    } else {
                        /* Otherwise, 'ansi' contains the color code */
                        while (*ansi)
                            *(to++) = (*(ansi++));
                    }
                }
            } else
                *(to++) = '^';
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* tct: This escapes Neon ANSI tags. I.e, ^RED^ -> ^^RED^^ */
char *
tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!out)
        perror("tct: Null buffer");

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 2)))
            if ((*(p++) = (*(in++))) == '^')
                *(p++) = '^';
    *p = '\0';
    return out;
}

/* This function strips out Neon ANSI tags. I.e., ^RED^ 
 * would be removed.
 */
char *
unparse_ansi(char *buf, const char *from)
{
    char *to;

    buf[0] = '\0';
    to = buf;
    while (*from) {
        if (*from == '^') {
            from++;

            if (*from == '^')
                *(to++) = '^';
            else
                while (*from && *from != '^')
                    from++;

            if (*from)
                from++;
        } else {
            *(to++) = (*(from++));
        }
    }
    *to = '\0';
    return buf;
}

/* This strips standard ANSI tags. I.e., \[[1;30m would be removed. */
char *
strip_ansi(char *buf, const char *input)
{
    const char *is;
    char *os;

    buf[0] = '\0';
    os = buf;

    is = input;

    while (*is) {
        if (*is == ESCAPE_CHAR) {
            is++;
            if (*is == '[') {
                is++;
                while (isdigit(*is) || *is == ';')
                    is++;
                if (*is == 'm')
                    is++;
            } else {
                is++;
            }
        } else {
            *os++ = *is++;
        }
    }
    *os = '\0';

    return buf;
}

/* This strips 256 COLOR ansi. I.e., \[[38;5;* codes will be removed.  */
char *
strip_256_ansi(char *buf, const char *input)
{
    const char *is = input;
    char *os = buf;
    buf[0] = '\0';
   
    while (*is) {
        if ( *is == ESCAPE_CHAR ) {
            if ( string_prefix(is, "\033[38;5;" ) || 
                 string_prefix(is, "\033[48;5;") ) {
                /* strip this code */
                is++;
                is++;
                while ( isdigit(*is) || *is == ';')
                    is++;
                if (*is == 'm')
                    is++;
           } else {
               *os++ = *is++;
           }
        } else {
           *os++ = *is++;
        }
    }
 
    *os = '\0';
 
    return buf;
}

/* strip_bad_ansi removes invalid ANSI tags from the string
 * before trying to notify it out. 
 */
char *
strip_bad_ansi(char *buf, const char *input)
{
    const char *is;
    char *os;
    int aflag = 0;

    buf[0] = '\0';
    os = buf;

    is = input;

    while (*is) {
        if (*is == ESCAPE_CHAR) {
            if (is[1] == '\0') {
                is++;
            } else if (is[1] != '[') {
                is++;
                is++;
            } else {
                aflag = 1;
                *os++ = *is++;  /* esc */
                *os++ = *is++;  /*  [  */
                while (isdigit(*is) || *is == ';') {
                    *os++ = *is++;
                }
                if (*is != 'm') {
                    *os++ = 'm';
                }
                *os++ = *is++;
            }
        } else {
            if ((*is == '\r') || (*is == '\n')) {
                while ((*is == '\r') || (*is == '\n'))
                    is++;
                if (!(*is) && (aflag)) {
                    *os++ = '\033';
                    *os++ = '[';
                    *os++ = '0';
                    *os++ = 'm';
                    aflag = 0;
                }
                *os++ = '\r';
                *os++ = '\n';
            } else {
                *os++ = *is++;
            }
        }
    }
    if (aflag) {
        *os++ = '\033';
        *os++ = '[';
        *os++ = '0';
        *os++ = 'm';
    }
    *os = '\0';

    return buf;
}

/* This escapes standard ANSI. I.e., \[[1;30m -> \\[[1;30m */
char *
escape_ansi(char *buf, const char *input)
{
    const char *is;
    char *os;

    is = input;
    buf[0] = '\0';
    os = buf;
    while (*is) {
        if (*is == ESCAPE_CHAR) {
            *os++ = '\\';
            *os++ = '[';
            (void) *is++;
        } else {
            *os++ = *is++;
        }
    }
    *os = '\0';

    return buf;
}

/* parse_mush_ansi converts MUSH ANSI tags into standard ANSI for
 * output. I.e, %cr -> \[[1;30m
 */
char *
parse_mush_ansi(char *buf, char *from)
{
    char *to, color, *ansi;

    to = buf;
    while (*from) {
        if (*from == '%' && (*(from+1) != '\0')) {
            (void) *from++;
            color = (*(from++));
            if (color == 'c') {
                color = (*(from++));
                switch (color) {
                    case 'x':
                        ansi = ANSICBLACK;
                        break;
                    case 'r':
                        ansi = ANSICRED;
                        break;
                    case 'g':
                        ansi = ANSICGREEN;
                        break;
                    case 'y':
                        ansi = ANSICYELLOW;
                        break;
                    case 'b':
                        ansi = ANSICBLUE;
                        break;
                    case 'm':
                        ansi = ANSICPURPLE;
                        break;
                    case 'c':
                        ansi = ANSICCYAN;
                        break;
                    case 'w':
                        ansi = ANSICWHITE;
                        break;
                    case 'X':
                        ansi = ANSIBBLACK;
                        break;
                    case 'R':
                        ansi = ANSIBRED;
                        break;
                    case 'G':
                        ansi = ANSIBGREEN;
                        break;
                    case 'Y':
                        ansi = ANSIBBROWN;
                        break;
                    case 'B':
                        ansi = ANSIBBLUE;
                        break;
                    case 'M':
                        ansi = ANSIBPURPLE;
                        break;
                    case 'C':
                        ansi = ANSIBCYAN;
                        break;
                    case 'W':
                        ansi = ANSIBGRAY;
                        break;
                    case 'i':
                    case 'I':
                        ansi = ANSIINVERT;
                        break;
                    case 'f':
                    case 'F':
                        ansi = ANSIFLASH;
                        break;
                    case 'h':
                    case 'H':
                        ansi = ANSIBOLD;
                        break;
                    case 'u':
                    case 'U':
                        ansi = ANSIUNDERLINE;
                        break;
                    default:
                        ansi = ANSINORMAL;
                        break;
                }
                if (*ansi)
                    while (*ansi)
                        *(to++) = (*(ansi++));
            } else {
                if (color == '%')
                    *(to++) = '%';
/*            if(*from) *from++; */
            }
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* unparse_mush_ansi: strip MUSH ANSI tags from a string.
 * I.e., %cr would be removed. 
 */
char *
unparse_mush_ansi(char *buf, char *from)
{
    char *to, color, *ansi;

    to = buf;
    while (*from) {
        if (*from == '%') {
            (void) *from++;
            color = (*(from++));
            if (color == 'c') {
                color = (*(from++));
                /* switch (color) {
                    default: */
                        ansi = "";
                        /* break;
                } */
                if (*ansi)
                    while (*ansi)
                        *(to++) = (*(ansi++));
            } else if (color == '%')
                *(to++) = '%';
/*         if(*from) *from++; */
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}


/* mush_tct: Escapes MUSH ANSI tags. I.e., %cr -> %%cr */
char *
mush_tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!(in) || !(out)) {
        return out;
    }

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 2)))
            if ((*(p++) = (*(in++))) == '%')
                *(p++) = '%';
    *p = '\0';
    return out;
}

/* parse_tilde_ansi: Convert FB/Glow style tilde ANSI into
 * standard ANSI for output. I.e., ~&110 -> \[[1;30m 
 */
char *
parse_tilde_ansi(char *buf, char *from)
{
    char *to, *ansi;
    int isbold = 0;

    to = buf;
    while (*from) {
        if ((*(from) == '~') && (*(from + 1) == '&')) {
            from += 2;
            if (!*from)
                break;

            if ((*(from) == '~') && (*(from + 1) == '&')) {
                /* Escape ~&~& into ~& */
                ansi = "~&";
                if (((to - buf) + strlen(ansi)) < BUFFER_LEN)
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from += 2;
                if (!*from)
                    break;
            } else if (TildeAnsiDigit(*from)) {
                char attr;

                /* ~&### pattern */
                if ((!from[1]) || (!from[2]) ||
                    (!TildeAnsiDigit(from[1])) || (!TildeAnsiDigit(from[2])))
                    continue;

                /* Check for bold or not in first digit. */
                attr = *from;
                isbold = (attr == '1') ? 1 : 0;
                from++;
                if (!*from)
                    break;      /* Just double checking */

                /* second position, foreground color */
                ansi = NULL;
                switch (*from) {
                    case '0':
                        ansi = (char *)(isbold ? ANSIGLOOM : ANSIBLACK);
                        break;
                    case '1':
                        ansi = (char *)(isbold ? ANSIRED : ANSICRIMSON);
                        break;
                    case '2':
                        ansi = (char *)(isbold ? ANSIGREEN : ANSIFOREST);
                        break;
                    case '3':
                        ansi = (char *)(isbold ? ANSIYELLOW : ANSIBROWN);
                        break;
                    case '4':
                        ansi = (char *)(isbold ? ANSIBLUE : ANSINAVY);
                        break;
                    case '5':
                        ansi = (char *)(isbold ? ANSIPURPLE : ANSIVIOLET);
                        break;
                    case '6':
                        ansi = (char *)(isbold ? ANSICYAN : ANSIAQUA);
                        break;
                    case '7':
                        ansi = (char *)(isbold ? ANSIWHITE : ANSIGRAY);
                        break;
                    case '-':
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));

                /* Take care of other first position attribute possibiliies. */
                ansi = NULL;
                switch (attr) {
                    case '2':  /* Need both to set invert, like old lib-ansi.muf */
                    case '8':
                        ansi = ANSIINVERT;
                        break;
                    case '5':  /* set for blinking foreground */
                        ansi = ANSIFLASH;
                        break;

                    case '-':  /* leave alone */
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;

                /* third and last position, background color */
                ansi = NULL;
                switch (*from) {
                    case '0':
                        ansi = ANSIBBLACK;
                        break;
                    case '1':
                        ansi = ANSIBRED;
                        break;
                    case '2':
                        ansi = ANSIBGREEN;
                        break;
                    case '3':
                        ansi = ANSIBBROWN;
                        break;
                    case '4':
                        ansi = ANSIBBLUE;
                        break;
                    case '5':
                        ansi = ANSIBPURPLE;
                        break;
                    case '6':
                        ansi = ANSIBCYAN;
                        break;
                    case '7':
                        ansi = ANSIBGRAY;
                        break;
                    case '-':
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;

            } else {
                /* The single letter attributes */
                ansi = NULL;
                switch (*from) {
                    case 'r':  /* RESET to normal colors. */
                    case 'R':
                        ansi = ANSINORMAL;
                        break;
                    case 'c':  /* this used to clear the screen, its retained */
                    case 'C':  /* for parsing only, doesnt actually do it.    */
                        ansi = "CLS";
                        break;
                    case 'b':  /* this is for BELL.. or CTRL-G */
                    case 'B':
                        ansi = "BEEP";
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;
            }
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';

    if ((to - buf) + strlen(ANSINORMAL) < BUFFER_LEN)
        strcat(to, ANSINORMAL);
    return buf;
}

/* tilde_striplen: Used in order to determine the # of characters
 * to remove when stripping tilde ANSI from a string.
 */
size_t
tilde_striplen(const char *word)
{
    const char *from;

    from = word;
    /* Technically, this test should never even have to be done. */
    if ((*(from + 0) == '~') && (*(from + 1) == '&') &&
        (*(from + 3) == '~') && (*(from + 4) == '&'))
        return 4;
    else if ((*(from + 0) == '~') && (*(from + 1) == '&')) {
        from += 2;
        if (*from) {
            if (TildeAnsiDigit(*from)) { /* Eat 3 digit pattern */
                if (from[1] && from[2] &&
                    TildeAnsiDigit(from[1]) && TildeAnsiDigit(from[2]))
                    from += 3;
            } else
                from++;         /* Eat 1 character pattern */
        }
    }
    return from - word;         /* Return the length of the ansi word. */
}

/* unparse_tilde_ansi: This removes tilde style ANSI tags from
 * a string. I.e., ~&110 would be removed.
 */
char *
unparse_tilde_ansi(char *buf, char *from)
{
    /* If escaped tilde ansi, take off first pair of ~& */
    /* Otherwise remove # of characters according to tilde_striplen */
    size_t count;
    char *to;

    to = buf;
    while (*from) {
        if ((*(from + 0) == '~') && (*(from + 1) == '&') &&
            (*(from + 2) == '~') && (*(from + 3) == '&')) {
            from += 2;
            *(to++) = (*(from++));
            *(to++) = (*(from++));
        } else if ((*from == '~') && (*(from + 1) == '&')) {
            count = tilde_striplen(from);
            if ((count > 0) && (count <= strlen(from)))
                from += count;
            else
                break;
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* tilde_tct: escapes tilde style ANSI tags. 
 * i.e., ~&110 -> ~~&110
 */
char *
tilde_tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!(in) || !(out)) {
        return out;
    }

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 3)))
            if (((*(p++) = (*(in++))) == '~') && ((*(p++) = (*(in++))) == '&')) {
                *(p++) = '~';
                *(p++) = '&';
            }
    *p = '\0';
    return out;
}

int
is_valid_pose_separator(char ch)
{
    return (ch == '\'') || (ch == ' ') || (ch == ',') || (ch == '-');
}


void
prefix_message(char *Dest, const char *Src, const char *Prefix,
               int BufferLength, int SuppressIfPresent)
{
    int PrefixLength = strlen(Prefix);
    int CheckForHangingEnter = 0;

    while ((BufferLength > PrefixLength) && (*Src != '\0')) {
        if (*Src == '\r') {
            Src++;
            continue;
        }

        if (!SuppressIfPresent || strncmp(Src, Prefix, PrefixLength)
            || (!is_valid_pose_separator(Src[PrefixLength])
                && (Src[PrefixLength] != '\r')
                && (Src[PrefixLength] != '\0'))) {

            strcpy(Dest, Prefix);

            Dest += PrefixLength;
            BufferLength -= PrefixLength;

            if (BufferLength > 1) {
                if (!is_valid_pose_separator(*Src)) {
                    *Dest++ = ' ';
                    BufferLength--;
                }
            }
        }

        while ((BufferLength > 1) && (*Src != '\0')) {
            *Dest++ = *Src;
            BufferLength--;

            if (*Src++ == '\r') {
                CheckForHangingEnter = 1;
                break;
            }
        }
    }

    if (CheckForHangingEnter && (Dest[-1] == '\r'))
        Dest--;

    *Dest = '\0';
}


int
is_prop_prefix(const char *Property, const char *Prefix)
{
    while (*Property == PROPDIR_DELIMITER)
        Property++;

    while (*Prefix == PROPDIR_DELIMITER)
        Prefix++;

    while (*Prefix) {
        if (*Property == '\0')
            return 0;
        if (*Property++ != *Prefix++)
            return 0;
    }

    return (*Property == '\0') || (*Property == PROPDIR_DELIMITER);
}

int
has_suffix(const char *text, const char *suffix)
{
    int tlen = text ? strlen(text) : 0;
    int slen = suffix ? strlen(suffix) : 0;

    if (!tlen || !slen || (tlen < slen))
        return 0;

    return !string_compare(text + tlen - slen, suffix);
}

int
has_suffix_char(const char *text, char suffix)
{
    int tlen = text ? strlen(text) : 0;

    if (tlen < 1)
        return 0;
    return text[tlen - 1] == suffix;
}

/* isascii_str():                                     */
/*   Checks all characters in a string with isascii() */
/*   and returns the result.                          */

bool
isascii_str(const char *str)
{
    const char *p;

    for (p = str; *p; p++)
        if (!isascii(*p))
            return 0;

    return 1;
}

char *
strcatn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = strlen(buf);
    char *dest = &buf[pos];

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }
    if (pos <= bufsize) {
        *dest = '\0';
    }
    return buf;
}

char *
strcpyn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = 0;
    char *dest = buf;

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return buf;
}

/* -*- mode: c; c-file-style: "k&r" -*-

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


/* partial change history:
 *
 * 2004-10-10 mbp: Lift out character type dependencies into macros.
 *
 * Eric Sosman pointed out that ctype functions take a parameter whose
 * value must be that of an unsigned int, even on platforms that have
 * negative chars in their default char type.
 */

/* #include <ctype.h> */
/* #include <string.h> */
/* #include <assert.h> */
/* #include <stdio.h> */

/* #include "strnatcmp.h" */


/* These are defined as macros to make it easier to adapt this code to
 * different characters types or comparison functions. */
static inline int
nat_isdigit(nat_char a)
{
     return isdigit((unsigned char) a);
}


static inline int
nat_isspace(nat_char a)
{
     return isspace((unsigned char) a);
}


static inline nat_char
nat_toupper(nat_char a)
{
     return toupper((unsigned char) a);
}



static int
compare_right(nat_char const *a, nat_char const *b)
{
     int bias = 0;
     
     /* The longest run of digits wins.  That aside, the greatest
	value wins, but we can't know that it will until we've scanned
	both numbers to know that they have the same magnitude, so we
	remember it in BIAS. */
     for (;; a++, b++) {
	  if (!nat_isdigit(*a)  &&  !nat_isdigit(*b))
	       return bias;
	  else if (!nat_isdigit(*a))
	       return -1;
	  else if (!nat_isdigit(*b))
	       return +1;
	  else if (*a < *b) {
	       if (!bias)
		    bias = -1;
	  } else if (*a > *b) {
	       if (!bias)
		    bias = +1;
	  } else if (!*a  &&  !*b)
	       return bias;
     }

     return 0;
}


static int
compare_left(nat_char const *a, nat_char const *b)
{
     /* Compare two left-aligned numbers: the first to have a
        different value wins. */
     for (;; a++, b++) {
	  if (!nat_isdigit(*a)  &&  !nat_isdigit(*b))
	       return 0;
	  else if (!nat_isdigit(*a))
	       return -1;
	  else if (!nat_isdigit(*b))
	       return +1;
	  else if (*a < *b)
	       return -1;
	  else if (*a > *b)
	       return +1;
     }
	  
     return 0;
}


static int strnatcmp0(nat_char const *a, nat_char const *b, int fold_case)
{
     int ai, bi;
     nat_char ca, cb;
     int fractional, result;
     
     /* assert(a && b); */
     ai = bi = 0;
     while (1) {
	  ca = a[ai]; cb = b[bi];

	  /* skip over leading spaces or zeros */
	  while (nat_isspace(ca))
	       ca = a[++ai];

	  while (nat_isspace(cb))
	       cb = b[++bi];

	  /* process run of digits */
	  if (nat_isdigit(ca)  &&  nat_isdigit(cb)) {
	       fractional = (ca == '0' || cb == '0');

	       if (fractional) {
		    if ((result = compare_left(a+ai, b+bi)) != 0)
			 return result;
	       } else {
		    if ((result = compare_right(a+ai, b+bi)) != 0)
			 return result;
	       }
	  }

	  if (!ca && !cb) {
	       /* The strings compare the same.  Perhaps the caller
                  will want to call strcmp to break the tie. */
	       return 0;
	  }

	  if (fold_case) {
	       ca = nat_toupper(ca);
	       cb = nat_toupper(cb);
	  }
	  
	  if (ca < cb)
	       return -1;
	  else if (ca > cb)
	       return +1;

	  ++ai; ++bi;
     }
}



int strnatcmp(nat_char const *a, nat_char const *b) {
     return strnatcmp0(a, b, 0);
}


/* Compare, recognizing numeric string and ignoring case. */
int strnatcasecmp(nat_char const *a, nat_char const *b) {
     return strnatcmp0(a, b, 1);
}


#ifdef UTF8_SUPPORT
/* mbstowcs and mblen take a passed in locale parameter under Windows, and
   that's not something I can research and test. Fix this if you need it.
   -brevantes */

/* Fetch the number of wide characters in a shared string. Only useful if the
   system locale supports multi-byte string characters. (such as UTF-8)
   If the string contains an illegal multi-byte string, this will return a
   value of -1. */
int
wcharlen(struct shared_string *ss)
{
    if (ss->wclength == -2) {
        /* -2 is uninitialized. */
        ss->wclength = mbstowcs(NULL, DoNullInd(ss), 0);
    }
    return ss->wclength;
}


/* count the number of bytes representing 'slice' wide characters in 'buf'.
   'buflen' is the strlen of buf, passed in to avoid recalculation. */
int
wcharlen_slice(char *buf, int slice, int buflen)
{
    char *cursor;
    int iter = 0;
    int result;
    size_t wcharlen = 0;

    cursor = buf;

    /* initialize shift state */
    mblen(NULL, 0);

    while ( iter != slice && *cursor != '\0'  ) {
        iter++;
        result = mblen(cursor, buflen - wcharlen);

        if (result == -1) {
            /* Illegal multibyte character detected. Return -1, let the caller
               figure out what to do. */
            return -1;
        }

        wcharlen = wcharlen + result;

        cursor = buf + (sizeof(char) * wcharlen);
    }

    return wcharlen;
}
#endif /* UTF8_SUPPORT */

