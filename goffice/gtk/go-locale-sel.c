/*
 * A locale selector widget.
 *
 *  Copyright (C) 2003 Andreas J. Guelzow
 *
 *  based on code by:
 *  Copyright (C) 2000 Marco Pesenti Gritti
 *  from the galeon code base
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <goffice/goffice-config.h>
#include "go-locale-sel.h"
#include "go-optionmenu.h"
#include "goffice/utils/go-glib-extras.h"
#include "goffice/utils/go-locale.h"
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>
#include <stdlib.h>

#define LS(x) GO_LOCALE_SEL (x)

#define LOCALE_NAME_KEY "Name of Locale"

/* ------------------------------------------------------------------------- */

typedef enum {
	LG_WESTERN_EUROPE,
	LG_EASTERN_EUROPE,
	LG_NORTH_AMERICA,
	LG_SOUTHCENTRAL_AMERICA,
	LG_ASIA,
	LG_MIDDLE_EAST,
	LG_AFRICA,
	LG_AUSTRALIA,
	LG_CARIBBEAN,
	LG_OTHER,
	LG_LAST
} LocaleGroup;

typedef struct
{
        char const *group_name;
	LocaleGroup const lgroup;
}
LGroupInfo;

static LGroupInfo lgroups[] = {
	{N_("Western Europe"), LG_WESTERN_EUROPE},
	{N_("Eastern Europe"), LG_EASTERN_EUROPE},
	{N_("North America"), LG_NORTH_AMERICA},
	{N_("South & Central America"), LG_SOUTHCENTRAL_AMERICA},
	{N_("Asia"), LG_ASIA},
	{N_("Middle East"), LG_MIDDLE_EAST},
	{N_("Africa"), LG_AFRICA},
	{N_("Australia"), LG_AUSTRALIA},
	{N_("Caribbean"), LG_CARIBBEAN},
	{N_("Other"), LG_OTHER},
	{NULL, LG_LAST}
};

static int
lgroups_order (const void *_a, const void *_b)
{
	const LGroupInfo *a = (const LGroupInfo *)_a;
	const LGroupInfo *b = (const LGroupInfo *)_b;

	return g_utf8_collate (_(a->group_name), _(b->group_name));
}

/* ------------------------------------------------------------------------- */

typedef struct {
	gchar const *locale_title;
	gchar const *base_locale;  /* "xx_XX" */
	LocaleGroup const lgroup;
	gboolean available;
	gchar *actual_locale;      /* "xx_XX" or "xx_XX.utf8" */
} LocaleInfo;

static LocaleInfo locale_trans_array[] = {
	/*
	 * The format here is "Country/Language (locale)" or just
	 * "Country (locale)" when there is only one choice or one
	 * very dominant language.
	 *
	 * Note: lots of people get very emotional over this.  Please
	 * err on the safe side, if any.
	 */
	{N_("United States/English (C)"),                "C",     LG_NORTH_AMERICA },
	{N_("Djibouti/Afar (aa_DJ)"),                    "aa_DJ", LG_AFRICA },
	{N_("Eritrea/Afar (aa_ER)"),                     "aa_ER", LG_AFRICA },
	{N_("Ethiopia/Afar (aa_ET)"),                    "aa_ET", LG_AFRICA },
	{N_("South Africa Afrikaans (af_ZA)"),           "af_ZA", LG_AFRICA },
	{N_("Ethiopia/Amharic (am_ET)"),                 "am_ET", LG_AFRICA },
	{N_("Spain/Aragonese (an_ES)"),                  "an_ES", LG_WESTERN_EUROPE },
	{N_("United Arab Emirates (ar_AE)"),             "ar_AE", LG_MIDDLE_EAST },
	{N_("Bahrain (ar_BH)"),                          "ar_BH", LG_MIDDLE_EAST },
	{N_("Algeria (ar_DZ)"),                          "ar_DZ", LG_AFRICA },
	{N_("Egypt (ar_EG)"),                            "ar_EG", LG_MIDDLE_EAST },
	{N_("India/Arabic (ar_IN)"),                     "ar_IN", LG_ASIA },
	{N_("Iraq (ar_IQ)"),                             "ar_IQ", LG_MIDDLE_EAST },
	{N_("Jordan (ar_JO)"),                           "ar_JO", LG_MIDDLE_EAST },
	{N_("Kuwait (ar_KW)"),                           "ar_KW", LG_MIDDLE_EAST },
	{N_("Lebanon (ar_LB)"),                          "ar_LB", LG_MIDDLE_EAST },
	{N_("Libya (ar_LY)"),                            "ar_LY", LG_AFRICA },
	{N_("Morocco (ar_MA)"),                          "ar_MA", LG_AFRICA },
	{N_("Oman (ar_OM)"),                             "ar_OM", LG_ASIA },
	{N_("Qatar (ar_QA)"),                            "ar_QA", LG_MIDDLE_EAST },
	{N_("Saudi Arabia (ar_SA)"),                     "ar_SA", LG_MIDDLE_EAST },
	{N_("Sudan (ar_SD)"),                            "ar_SD", LG_AFRICA },
	{N_("Syria (ar_SY)"),                            "ar_SY", LG_MIDDLE_EAST },
	{N_("Tunisia (ar_TN)"),                          "ar_TN", LG_AFRICA },
	{N_("Yemen (ar_YE)"),                            "ar_YE", LG_MIDDLE_EAST },
	{N_("India/Assamese (as_IN)"),                   "as_IN", LG_ASIA },
	{N_("Spain/Asturian (ast_ES)"),                  "ast_ES", LG_WESTERN_EUROPE },
	{N_("Azerbaijan (az_AZ)"),                       "az_AZ", LG_ASIA },
	{N_("Belarus (be_BY)"),                          "be_BY", LG_EASTERN_EUROPE },
	{N_("Zambia/Bemba (bem_ZM)"),                    "bem_ZM", LG_AFRICA },
	{N_("Algeria/Amazigh (ber_DZ)"),                 "ber_DZ", LG_AFRICA },
	{N_("Morocco/Amazigh (ber_MA)"),                 "ber_MA", LG_AFRICA },
	{N_("Bulgaria (bg_BG)"),                         "bg_BG", LG_EASTERN_EUROPE },
	{N_("Bangladesh (bn_BD)"),                       "bn_BD", LG_ASIA },
	{N_("India/Bengali (bn_IN)"),                    "bn_IN", LG_ASIA },
	{N_("China/Tibetan (bo_CN)"),                    "bo_CN", LG_ASIA },
	{N_("India/Tibetan (bo_IN)"),                    "bo_IN", LG_ASIA },
	{N_("France/Breton (br_FR)"),                    "br_FR", LG_WESTERN_EUROPE },
	{N_("Bosnia and Herzegowina (bs_BA)"),           "bs_BA", LG_EASTERN_EUROPE },
	{N_("Eritrea/Blin (byn_ER)"),                    "byn_ER", LG_AFRICA },
	{N_("Andorra/Catalan (ca_AD)"),                  "ca_AD", LG_WESTERN_EUROPE },
	{N_("Spain/Catalan (ca_ES)"),                    "ca_ES", LG_WESTERN_EUROPE },
	{N_("France/Catalan (ca_FR)"),                   "ca_FR", LG_WESTERN_EUROPE },
	{N_("Italy/Catalan (ca_IT)"),                    "ca_IT", LG_WESTERN_EUROPE },
	{N_("Ukraine/Crimean Tatar (crh_UA)"),           "crh_UA", LG_EASTERN_EUROPE },
	{N_("Czech Republic (cs_CZ)"),                   "cs_CZ", LG_EASTERN_EUROPE },
	{N_("Poland/Kashubian (csb_PL)"),                "csb_PL", LG_EASTERN_EUROPE },
	{N_("Russia/Chuvash (cv_RU)"),                   "cv_RU", LG_EASTERN_EUROPE },
	{N_("Great Britain/Welsh (cy_GB)"),              "cy_GB", LG_WESTERN_EUROPE },
	{N_("Denmark (da_DK)"),                          "da_DK", LG_WESTERN_EUROPE },
	{N_("Austria (de_AT)"),                          "de_AT", LG_WESTERN_EUROPE },
	{N_("Belgium/German (de_BE)"),                   "de_BE", LG_WESTERN_EUROPE },
	{N_("Switzerland/German (de_CH)"),               "de_CH", LG_WESTERN_EUROPE },
	{N_("Germany (de_DE)"),                          "de_DE", LG_WESTERN_EUROPE },
	{N_("Luxembourg/German (de_LU)"),                "de_LU", LG_WESTERN_EUROPE },
	{N_("Maldives (dv_MV)"),                         "dv_MV", LG_ASIA },
	{N_("Bhutan (dz_BT)"),                           "dz_BT", LG_ASIA },
	{N_("Cyprus/Greek (el_CY)"),                     "el_CY", LG_WESTERN_EUROPE },
	{N_("Greece (el_GR)"),                           "el_GR", LG_WESTERN_EUROPE },
	{N_("Antigua and Barbuda/English (en_AG)"),      "en_AG", LG_CARIBBEAN },
	{N_("Australia (en_AU)"),                        "en_AU", LG_AUSTRALIA },
	{N_("Botswana (en_BW)"),                         "en_BW", LG_AFRICA },
	{N_("Canada/English (en_CA)"),                   "en_CA", LG_NORTH_AMERICA },
	{N_("Great Britain (en_GB)"),                    "en_GB", LG_WESTERN_EUROPE },
	{N_("Hong Kong/English (en_HK)"),                "en_HK", LG_ASIA },
	{N_("Ireland (en_IE)"),                          "en_IE", LG_WESTERN_EUROPE },
	{N_("India/English (en_IN)"),                    "en_IN", LG_ASIA },
	{N_("New Zealand (en_NZ)"),                      "en_NZ", LG_AUSTRALIA },
	{N_("Philippines (en_PH)"),                      "en_PH", LG_ASIA },
	{N_("Singapore/English (en_SG)"),                "en_SG", LG_ASIA },
	{N_("United States/English (en_US)"),            "en_US", LG_NORTH_AMERICA },
	{N_("South Africa/English (en_ZA)"),             "en_ZA", LG_AFRICA },
	{N_("Zimbabwe (en_ZW)"),                         "en_ZW", LG_AFRICA },
	{N_("Esperanto (eo_EO)"),                        "eo_EO", LG_OTHER },
	{N_("Argentina (es_AR)"),                        "es_AR", LG_SOUTHCENTRAL_AMERICA },
	{N_("Bolivia (es_BO)"),                          "es_BO", LG_SOUTHCENTRAL_AMERICA },
	{N_("Chile (es_CL)"),                            "es_CL", LG_SOUTHCENTRAL_AMERICA },
	{N_("Colombia (es_CO)"),                         "es_CO", LG_SOUTHCENTRAL_AMERICA },
	{N_("Costa Rica (es_CR)"),                       "es_CR", LG_CARIBBEAN },
	{N_("Dominican Republic (es_DO)"),               "es_DO", LG_CARIBBEAN },
	{N_("Ecuador (es_EC)"),                          "es_EC", LG_SOUTHCENTRAL_AMERICA },
	{N_("Spain (es_ES)"),                            "es_ES", LG_WESTERN_EUROPE },
	{N_("Guatemala (es_GT)"),                        "es_GT", LG_SOUTHCENTRAL_AMERICA },
	{N_("Honduras (es_HN)"),                         "es_HN", LG_SOUTHCENTRAL_AMERICA },
	{N_("Mexico (es_MX)"),                           "es_MX", LG_SOUTHCENTRAL_AMERICA },
	{N_("Nicaragua (es_NI)"),                        "es_NI", LG_SOUTHCENTRAL_AMERICA },
	{N_("Panama (es_PA)"),                           "es_PA", LG_SOUTHCENTRAL_AMERICA },
	{N_("Peru (es_PE)"),                             "es_PE", LG_SOUTHCENTRAL_AMERICA },
	{N_("Puerto Rico (es_PR)"),                      "es_PR", LG_CARIBBEAN },
	{N_("Paraguay (es_PY)"),                         "es_PY", LG_SOUTHCENTRAL_AMERICA },
	{N_("El Salvador (es_SV)"),                      "es_SV", LG_SOUTHCENTRAL_AMERICA },
	{N_("United States/Spanish (es_US)"),            "es_US", LG_NORTH_AMERICA },
	{N_("Uruguay (es_UY)"),                          "es_UY", LG_SOUTHCENTRAL_AMERICA },
	{N_("Venezuela (es_VE)"),                        "es_VE", LG_SOUTHCENTRAL_AMERICA },
	{N_("Estonia (et_EE)"),                          "et_EE", LG_EASTERN_EUROPE },
	{N_("Spain/Basque (eu_ES)"),                     "eu_ES", LG_WESTERN_EUROPE },
	{N_("Iran (fa_IR)"),                             "fa_IR", LG_MIDDLE_EAST },
	{N_("Senegal/Fulah (ff_SN)"),                    "ff_SN", LG_AFRICA },
	{N_("Finland/Finnish (fi_FI)"),                  "fi_FI", LG_WESTERN_EUROPE },
	{N_("Philippines/Filipino (fil_PH)"),            "fil_PH", LG_ASIA },
	{N_("Faroe Islands (fo_FO)"),                    "fo_FO", LG_WESTERN_EUROPE },
	{N_("Belgium/French (fr_BE)"),                   "fr_BE", LG_WESTERN_EUROPE },
	{N_("Canada/French (fr_CA)"),                    "fr_CA", LG_NORTH_AMERICA },
	{N_("Switzerland/French (fr_CH)"),               "fr_CH", LG_WESTERN_EUROPE },
	{N_("France (fr_FR)"),                           "fr_FR", LG_WESTERN_EUROPE },
	{N_("Luxembourg/French (fr_LU)"),                "fr_LU", LG_WESTERN_EUROPE },
	{N_("Italy/Furlan (fur_IT)"),                    "fur_IT", LG_WESTERN_EUROPE },
	{N_("Germany/Frisian (fy_DE)"),                  "fy_DE", LG_WESTERN_EUROPE },
	{N_("Netherlands/Frisian (fy_NL)"),              "fy_NL", LG_WESTERN_EUROPE },
	{N_("Ireland/Gaelic (ga_IE)"),                   "ga_IE", LG_WESTERN_EUROPE },
	{N_("Great Britain/Scottish Gaelic (gd_GB)"),    "gd_GB", LG_WESTERN_EUROPE },
	{N_("Eritrea/Ge'ez (gez_ER)"),                   "gez_ER", LG_AFRICA },
	{N_("Ethiopia/Ge'ez (gez_ET)"),                  "gez_ET", LG_AFRICA },
	{N_("Spain/Galician (gl_ES)"),                   "gl_ES", LG_WESTERN_EUROPE },
	{N_("India/Gujarati (gu_IN)"),                   "gu_IN", LG_ASIA },
	{N_("Great Britain/Manx Gaelic (gv_GB)"),        "gv_GB", LG_WESTERN_EUROPE },
	{N_("Nigeria/Hausa (ha_NG)"),                    "ha_NG", LG_AFRICA },
	{N_("Israel/Hebrew (he_IL)"),                    "he_IL", LG_MIDDLE_EAST },
	{N_("India/Hindu (hi_IN)"),                      "hi_IN", LG_ASIA },
	{N_("India/Chhattisgarhi (hne_IN)"),             "hne_IN", LG_ASIA },
	{N_("Croatia (hr_HR)"),                          "hr_HR", LG_EASTERN_EUROPE },
	{N_("Germany/Upper Sorbian (hsb_DE)"),           "hsb_DE", LG_WESTERN_EUROPE },
	{N_("Haiti/Kreyol (ht_HT)"),                     "ht_HT", LG_CARIBBEAN },
	{N_("Hungary (hu_HU)"),                          "hu_HU", LG_EASTERN_EUROPE },
	{N_("Armenia (hy_AM)"),                          "hy_AM", LG_EASTERN_EUROPE },
	{N_("(i18n)"),                                   "i18n",  LG_OTHER },
	{N_("Indonesia (id_ID)"),                        "id_ID", LG_ASIA },
	{N_("Nigeria/Igbo (ig_NG)"),                     "ig_NG", LG_AFRICA },
	{N_("Canada/Inupiaq (ik_CA)"),                   "ik_CA", LG_NORTH_AMERICA },
	{N_("Iceland (is_IS)"),                          "is_IS", LG_WESTERN_EUROPE },
	{N_("(iso14651_t1)"),                            "iso14651_t1", LG_OTHER },
	{N_("Switzerland/Italian (it_CH)"),              "it_CH", LG_WESTERN_EUROPE },
	{N_("Italy (it_IT)"),                            "it_IT", LG_WESTERN_EUROPE },
	{N_("Canada/Inuktitut (iu_CA)"),                 "iu_CA", LG_NORTH_AMERICA },
	{N_("Japan (ja_JP)"),                            "ja_JP", LG_ASIA },
	{N_("Georgia (ka_GE)"),                          "ka_GE", LG_EASTERN_EUROPE },
	{N_("Kazakhstan/Kazakh (kk_KZ)"),                "kk_KZ", LG_ASIA },
	{N_("Greenland (kl_GL)"),                        "kl_GL", LG_WESTERN_EUROPE },
	{N_("Cambodia/Khmer (km_KH)"),                   "km_KH", LG_ASIA },
	{N_("India/Kannada (kn_IN)"),                    "kn_IN", LG_ASIA },
	{N_("Korea (ko_KR)"),                            "ko_KR", LG_ASIA },
	{N_("India/Konkani (kok_IN)"),                   "kok_IN", LG_ASIA },
	{N_("India/Kashmiri (ks_IN)"),                   "ks_IN", LG_ASIA },
	{N_("Turkey/Kurdish (ku_TR)"),                   "ku_TR", LG_ASIA },
	{N_("Great Britain/Cornish (kw_GB)"),            "kw_GB", LG_WESTERN_EUROPE },
	{N_("Kyrgyzstan/Kyrgyz (ky_KG)"),                "ky_KG", LG_ASIA },
	{N_("Luxembourg/Luxembourgish (lb_LU)"),         "lb_LU", LG_WESTERN_EUROPE },
	{N_("Uganda/Luganda (lg_UG)"),                   "lg_UG", LG_AFRICA },
	{N_("Belgium/Limburgish (li_BE)"),               "li_BE", LG_WESTERN_EUROPE },
	{N_("Netherlands/Limburgish (li_NL)"),           "li_NL", LG_WESTERN_EUROPE },
	{N_("Italy/Ligurian (lij_IT)"),                  "lij_IT", LG_WESTERN_EUROPE },
	{N_("Laos (lo_LA)"),                             "lo_LA", LG_ASIA },
	{N_("Lithuania (lt_LT)"),                        "lt_LT", LG_EASTERN_EUROPE },
	{N_("Latvia (lv_LV)"),                           "lv_LV", LG_EASTERN_EUROPE },
	{N_("India/Maithili (mai_IN)"),                  "mai_IN", LG_ASIA },
	{N_("Madagascar/Malagasy (mg_MG)"),              "mg_MG", LG_AFRICA },
	{N_("Russia/Mari (mhr_RU)"),                     "mhr_RU", LG_EASTERN_EUROPE },
	{N_("New Zealand/Maori (mi_NZ)"),                "mi_NZ", LG_AUSTRALIA },
	{N_("Macedonia (mk_MK)"),                        "mk_MK", LG_EASTERN_EUROPE },
	{N_("India/Malayalam (ml_IN)"),                  "ml_IN", LG_ASIA },
	{N_("Mongolia (mn_MN)"),                         "mn_MN", LG_ASIA },
	{N_("India/Marathi (mr_IN)"),                    "mr_IN", LG_ASIA },
	{N_("Malaysia (ms_MY)"),                         "ms_MY", LG_ASIA },
	{N_("Malta (mt_MT)"),                            "mt_MT", LG_WESTERN_EUROPE },
	{N_("Myanmar/Burmese (my_MM)"),                  "my_MM", LG_ASIA },
	{N_("Taiwan/Minnan (nan_TW@latin)"),             "nan_TW@latin", LG_ASIA },
	{N_("Norway/Bokmal (nb_NO)"),                    "nb_NO", LG_WESTERN_EUROPE },
	{N_("Germany/Low Saxon (nds_DE)"),               "nds_DE", LG_WESTERN_EUROPE },
	{N_("Netherlands/Low Saxon (nds_NL)"),           "nds_NL", LG_WESTERN_EUROPE },
	{N_("Nepal/Nepali (ne_NP)"),                     "ne_NP", LG_ASIA },
	{N_("Aruba/Dutch (nl_AW)"),                      "nl_AW", LG_CARIBBEAN },
	{N_("Belgium/Flemish (nl_BE)"),                  "nl_BE", LG_WESTERN_EUROPE },
	{N_("The Netherlands (nl_NL)"),                  "nl_NL", LG_WESTERN_EUROPE },
	{N_("Norway/Nynorsk (nn_NO)"),                   "nn_NO", LG_WESTERN_EUROPE },
	{N_("South Africa/Southern Ndebele (nr_ZA)"),    "nr_ZA", LG_AFRICA },
	{N_("South Africa/Northern Sotho (nso_ZA)"),     "nso_ZA", LG_AFRICA },
	{N_("France/Occitan (oc_FR)"),                   "oc_FR", LG_WESTERN_EUROPE },
	{N_("Ethiopia/Oromo (om_ET)"),                   "om_ET", LG_AFRICA },
	{N_("Kenya/Oromo (om_KE)"),                      "om_KE", LG_AFRICA },
	{N_("India/Oriya (or_IN)"),                      "or_IN", LG_ASIA },
	{N_("Russia/Ossetian (os_RU)"),                  "os_RU", LG_EASTERN_EUROPE },
	{N_("India/Punjabi (Gurmukhi) (pa_IN)"),         "pa_IN", LG_ASIA },
	{N_("Pakistan/Punjabi (Shamukhi) (pa_PK)"),      "pa_PK", LG_ASIA },
	{N_("Netherland Antilles/Papiamento (pap_AN)"),  "pap_AN", LG_CARIBBEAN },
	{N_("Poland (pl_PL)"),                           "pl_PL", LG_EASTERN_EUROPE },
	{N_("Afghanistan/Pashto (ps_AF)"),               "ps_AF", LG_ASIA },
	{N_("Brazil (pt_BR)"),                           "pt_BR", LG_SOUTHCENTRAL_AMERICA },
	{N_("Portugal (pt_PT)"),                         "pt_PT", LG_WESTERN_EUROPE },
	{N_("Romania (ro_RO)"),                          "ro_RO", LG_EASTERN_EUROPE },
	{N_("Russia (ru_RU)"),                           "ru_RU", LG_EASTERN_EUROPE },
	{N_("Ukraine/Russian (ru_UA)"),                  "ru_UA", LG_EASTERN_EUROPE },
	{N_("Rwanda/Kinyarwanda (rw_RW)"),               "rw_RW", LG_AFRICA },
	{N_("India/Sanskrit (sa_IN)"),                   "sa_IN", LG_ASIA },
	{N_("Italy/Sardinian (sc_IT)"),                  "sc_IT", LG_WESTERN_EUROPE },
	{N_("India/Sindhi (sd_IN)"),                     "sd_IN", LG_ASIA },
	{N_("Norway/Saami (se_NO)"),                     "se_NO", LG_WESTERN_EUROPE },
	{N_("Canada/Secwepemctsin (Shuswap) (shs_CA)"),  "shs_CA", LG_NORTH_AMERICA },
	{N_("Sri Lanka/Sinhala (si_LK)"),                "si_LK", LG_ASIA },
	{N_("Ethiopia/Sidama (sid_ET)"),                 "sid_ET", LG_AFRICA },
	{N_("Slovakia (sk_SK)"),                         "sk_SK", LG_EASTERN_EUROPE },
	{N_("Slovenia (sl_SI)"),                         "sl_SI", LG_EASTERN_EUROPE },
	{N_("Djibouti/Somali (so_DJ)"),                  "so_DJ", LG_AFRICA },
	{N_("Ethiopia/Somali (so_ET)"),                  "so_ET", LG_AFRICA },
	{N_("Kenya/Somali (so_KE)"),                     "so_KE", LG_AFRICA },
	{N_("Somalia/Somali (so_SO)"),                   "so_SO", LG_AFRICA },
	{N_("Albania (sq_AL)"),                          "sq_AL", LG_EASTERN_EUROPE },
	{N_("Macedonia/Albanian (sq_MK)"),               "sq_MK", LG_EASTERN_EUROPE },
	{N_("Montenegro/Serbian (sr_ME)"),               "sr_ME", LG_EASTERN_EUROPE },
	{N_("Yugoslavia (sr_RS)"),                       "sr_RS", LG_EASTERN_EUROPE },
	{N_("South Africa/Swati (ss_ZA)"),               "ss_ZA", LG_AFRICA },
	{N_("South Africa/Sotho (st_ZA)"),               "st_ZA", LG_AFRICA },
	{N_("Finland/Swedish (sv_FI)"),                  "sv_FI", LG_WESTERN_EUROPE },
	{N_("Sweden (sv_SE)"),                           "sv_SE", LG_WESTERN_EUROPE },
	{N_("Kenya/Swahili (sw_KE)"),                    "sw_KE", LG_AFRICA },
	{N_("Tanzania/Swahili (sw_TZ)"),                 "sw_TZ", LG_AFRICA },
	{N_("India/Tamil (ta_IN)"),                      "ta_IN", LG_ASIA },
	{N_("India/Telugu (te_IN)"),                     "te_IN", LG_ASIA },
	{N_("Tajikistan (tg_TJ)"),                       "tg_TJ", LG_ASIA },
	{N_("Thailand (th_TH)"),                         "th_TH", LG_ASIA },
	{N_("Eritrea/Tigrinya (ti_ER)"),                 "ti_ER", LG_AFRICA },
	{N_("Ethiopia/Tigrinya (ti_ET)"),                "ti_ET", LG_AFRICA },
	{N_("Eritrea/Tigre (tig_ER)"),                   "tig_ER", LG_AFRICA },
	{N_("Turkmenistan /Turkmen (tk_TM)"),            "tk_TM", LG_ASIA },
	{N_("Philippines/Tagalog (tl_PH)"),              "tl_PH", LG_ASIA },
	{N_("South Africa/Tswana (tn_ZA)"),              "tn_ZA", LG_AFRICA },
	{N_("Cyprus/Turkish (tr_CY)"),                   "tr_CY", LG_WESTERN_EUROPE },
	{N_("Turkey (tr_TR)"),                           "tr_TR", LG_ASIA },
	{N_("South Africa/Tsonga (ts_ZA)"),              "ts_ZA", LG_AFRICA },
	{N_("Russia/Tatar (tt_RU)"),                     "tt_RU", LG_EASTERN_EUROPE },
	{N_("China/Uyghur (ug_CN)"),                     "ug_CN", LG_ASIA },
	{N_("Ukraine (uk_UA)"),                          "uk_UA", LG_EASTERN_EUROPE },
	{N_("India/Urdu (ur_IN)"),                       "ur_IN", LG_ASIA },
	{N_("Pakistan (ur_PK)"),                         "ur_PK", LG_ASIA },
	{N_("Uzbekistan (uz_UZ)"),                       "uz_UZ", LG_ASIA },
	{N_("South Africa/Venda (ve_ZA)"),               "ve_ZA", LG_AFRICA },
	{N_("Vietnam (vi_VN)"),                          "vi_VN", LG_ASIA },
	{N_("Belgium/Walloon (wa_BE)"),                  "wa_BE", LG_WESTERN_EUROPE },
	{N_("Switzerland/Walser (wae_CH)"),              "wae_CH", LG_WESTERN_EUROPE },
	{N_("Ethiopia/Wollayta (wal_ET)"),                "wal_ET", LG_AFRICA },
	{N_("Senegal/Wolof (wo_SN)"),                    "wo_SN", LG_AFRICA },
	{N_("South Africa/Xhosa (xh_ZA)"),               "xh_ZA", LG_AFRICA },
	{N_("United States/Yiddish (yi_US)"),            "yi_US", LG_NORTH_AMERICA },
	{N_("Nigeria/Yoruba (yo_NG)"),                   "yo_NG", LG_AFRICA },
	{N_("Hong Kong/Cantonese (Yue) (yue_HK)"),       "yue_HK", LG_ASIA },
	{N_("China (zh_CN)"),                            "zh_CN", LG_ASIA },
	{N_("Hong Kong/Chinese (zh_HK)"),                "zh_HK", LG_ASIA },
	{N_("Singapore/Chinese (zh_SG)"),                "zh_SG", LG_ASIA },
	{N_("Taiwan (zh_TW)"),                           "zh_TW", LG_ASIA },
	{N_("South Africa/Zulu (zu_ZA)"),                "zu_ZA", LG_AFRICA },
	{NULL,                                           NULL,    LG_LAST}
};

/* What is this?  See also iw_IL. {N_("(he_IL)"),"he_IL", LG_ASIA }, */
/* {N_("(lug_UG)"),                                 "lug_UG",LG_OTHER }, */


static int
locale_order (const void *_a, const void *_b)
{
	const LocaleInfo *a = (const LocaleInfo *)_a;
	const LocaleInfo *b = (const LocaleInfo *)_b;

	if (a->lgroup != b->lgroup)
		return b->lgroup - a->lgroup;

	return g_utf8_collate (_(a->locale_title), _(b->locale_title));
}

/* ------------------------------------------------------------------------- */

/* name -> LocaleInfo* mapping */
static GHashTable *locale_hash;

struct _GOLocaleSel {
	GtkBox box;
	GOOptionMenu *locales;
	GtkMenu *locales_menu;
};

typedef struct {
	GtkBoxClass parent_class;

	gboolean (* locale_changed) (GOLocaleSel *ls, char const *new_locale);
} GOLocaleSelClass;


typedef GOLocaleSel Ls;
typedef GOLocaleSelClass LsClass;

/* Signals we emit */
enum {
	LOCALE_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0
};




static guint ls_signals[LAST_SIGNAL] = { 0 };

static void ls_set_property      (GObject          *object,
				  guint             prop_id,
				  const GValue     *value,
				  GParamSpec       *pspec);

static void ls_get_property      (GObject          *object,
				  guint             prop_id,
				  GValue           *value,
				  GParamSpec       *pspec);

const char *
go_locale_sel_get_locale_name (G_GNUC_UNUSED GOLocaleSel *ls,
			       const char *locale)
{
	LocaleInfo const *ci;

	g_return_val_if_fail (locale != NULL, NULL);

	ci = g_hash_table_lookup (locale_hash, locale);
	return ci ? _(ci->locale_title) : NULL;
}

static char *
get_locale_name (GOLocaleSel *ls)
{
	char const *cur_locale, *name;
	char *locale, *p;
	const char *ellipsis = "...";

	/*
	 * We cannot use LC_ALL here because a composite locale may have
	 * a string that is a mile wide (and not be intented for humans
	 * anyway).  Why use LC_MESSAGES?  Good question, but in actuality
	 * I doubt it will matter.  It's an arbitrary choice.
	 */
	cur_locale = setlocale (LC_MESSAGES, NULL);
	if (!cur_locale) cur_locale = "C";  /* Just in case.  */
	locale = g_strdup (cur_locale);

	name = go_locale_sel_get_locale_name (ls, locale);
	if (name)
		goto gotit;

	p = strchr (locale, '.');
	if (p) {
		strcpy (p, ".utf8");
		name = go_locale_sel_get_locale_name (ls, locale);
		if (name)
			goto gotit;
		*p = 0;
	} else {
		p = g_strconcat (locale, ".utf8", NULL);
		name = go_locale_sel_get_locale_name (ls, p);
		if (name) {
			g_free (locale);
			locale = p;
			goto gotit;
		}
		g_free (p);
	}

	p = strchr (locale, '@');
	if (p)
		*p = 0;

	name = go_locale_sel_get_locale_name (ls, locale);
	if (name)
		goto gotit;

	/* Just in case we get something really wide.  */
	if ((size_t)g_utf8_strlen (locale, -1) > 50 + strlen (ellipsis))
		strcpy (g_utf8_offset_to_pointer (locale, 50), ellipsis);
	return locale;

 gotit:
	g_free (locale);
	return g_strdup (name);
}

static void
locales_changed_cb (GOOptionMenu *optionmenu, GOLocaleSel *ls)
{
	char * locale;

	g_return_if_fail (GO_IS_LOCALE_SEL (ls));
	g_return_if_fail (optionmenu == ls->locales);

	locale = go_locale_sel_get_locale (ls);

	g_signal_emit (G_OBJECT (ls),
		       ls_signals[LOCALE_CHANGED],
		       0, locale);
	g_free (locale);
}

static void
set_menu_to_default (GOLocaleSel *ls, gint item)
{
	GSList sel = { GINT_TO_POINTER (item - 1), NULL};

	g_return_if_fail (ls != NULL && GO_IS_LOCALE_SEL (ls));

	go_option_menu_set_history (ls->locales, &sel);
}

static gboolean
ls_mnemonic_activate (GtkWidget *w, gboolean group_cycling)
{
	GOLocaleSel *ls = GO_LOCALE_SEL (w);
	gtk_widget_grab_focus (GTK_WIDGET (ls->locales));
	return TRUE;
}


static void
ls_build_menu (GOLocaleSel *ls)
{
        GtkWidget *item;
	GtkMenu *menu;
	LGroupInfo const *lgroup = lgroups;
	gint lg_cnt = 0;

        menu = GTK_MENU (gtk_menu_new ());

	while (lgroup->group_name) {
		LocaleInfo const *locale_trans;
		GtkMenu *submenu = NULL;

		locale_trans = locale_trans_array;

		while (locale_trans->lgroup != LG_LAST) {
			if (locale_trans->lgroup == lgroup->lgroup && locale_trans->available) {
				GtkWidget *subitem=
					gtk_check_menu_item_new_with_label
					(_(locale_trans->locale_title));
				if (!submenu)
					submenu = GTK_MENU (gtk_menu_new ());

				gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (subitem), TRUE);
				gtk_widget_show (subitem);
				gtk_menu_shell_append (GTK_MENU_SHELL (submenu),  subitem);
				g_object_set_data (G_OBJECT (subitem), LOCALE_NAME_KEY,
						   (locale_trans->actual_locale));
			}
			locale_trans++;
		}
		if (submenu) {
			GtkWidget *item = gtk_menu_item_new_with_label (_(lgroup->group_name));
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
						   GTK_WIDGET (submenu));
			gtk_widget_show (item);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			lg_cnt++;
		}
                lgroup++;
        }
	item = gtk_separator_menu_item_new ();
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu),  item);
	lg_cnt++;

	{
		char *locale_name = get_locale_name (ls);
		char *locale_menu_title = g_strconcat (_("Current Locale: "),
						       locale_name, NULL);
		g_free (locale_name);
		item = gtk_check_menu_item_new_with_label (locale_menu_title);
		gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (item), TRUE);
		g_free (locale_menu_title);
		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu),  item);
		lg_cnt++;
	}

	go_option_menu_set_menu (ls->locales, GTK_WIDGET (menu));
	ls->locales_menu = menu;
	set_menu_to_default (ls, lg_cnt);
}

static void
ls_init (GOLocaleSel *ls)
{
	ls->locales = GO_OPTION_MENU (go_option_menu_new ());
	ls_build_menu (ls);

	g_signal_connect (G_OBJECT (ls->locales), "changed",
                          G_CALLBACK (locales_changed_cb), ls);
        gtk_box_pack_start (GTK_BOX (ls), GTK_WIDGET (ls->locales),
                            TRUE, TRUE, 0);
}

static void
ls_class_init (GtkWidgetClass *widget_klass)
{
	LocaleInfo *ci;
	char *oldlocale;

	GObjectClass *gobject_class = G_OBJECT_CLASS (widget_klass);
	widget_klass->mnemonic_activate = ls_mnemonic_activate;

	gobject_class->set_property = ls_set_property;
	gobject_class->get_property = ls_get_property;

	ls_signals[LOCALE_CHANGED] =
		g_signal_new ("locale_changed",
			      GO_TYPE_LOCALE_SEL,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GOLocaleSelClass, locale_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	qsort (lgroups, G_N_ELEMENTS (lgroups) - 2, sizeof (lgroups[0]),
	       lgroups_order);
	qsort (locale_trans_array, G_N_ELEMENTS (locale_trans_array) - 1,
	       sizeof (locale_trans_array[0]), locale_order);

	locale_hash =
		g_hash_table_new_full (go_ascii_strcase_hash,
				       go_ascii_strcase_equal,
				       NULL,
				       NULL);

	oldlocale = g_strdup (setlocale (LC_ALL, NULL));
	for (ci = locale_trans_array; ci->locale_title; ci++) {
		const char *locale = ci->base_locale;
		char *localeutf8 = g_strconcat (locale, ".utf8", NULL);
		ci->available = (setlocale (LC_ALL, localeutf8) != NULL);
		if (ci->available) {
			ci->actual_locale = localeutf8;
		} else {
			ci->available = (setlocale (LC_ALL, locale) != NULL);
			ci->actual_locale = g_strdup (locale);
			g_free (localeutf8);
		}
		g_hash_table_insert (locale_hash, ci->actual_locale, ci);
	}

	/* Handle the POSIX/C alias.  */
	ci = g_hash_table_lookup (locale_hash, "C");
	if (ci)
		g_hash_table_insert (locale_hash, (char *)"POSIX", ci);

	setlocale (LC_ALL, oldlocale);
	g_free (oldlocale);

#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name (widget_klass, "localeselector");
#endif
}

GSF_CLASS (GOLocaleSel, go_locale_sel,
	   ls_class_init, ls_init, GTK_TYPE_BOX)

GtkWidget *
go_locale_sel_new (void)
{
	return g_object_new (GO_TYPE_LOCALE_SEL, NULL);
}

gchar *
go_locale_sel_get_locale (GOLocaleSel *ls)
{
	GtkMenuItem *selection;
	char const *cur_locale;
	char const *locale;

	char *cur_locale_cp = NULL;
	char **parts;

	cur_locale = setlocale (LC_ALL, NULL);
	if (cur_locale) {
		parts = g_strsplit (cur_locale, ".", 2);
		cur_locale_cp = g_strdup (parts[0]);
		g_strfreev (parts);
	}

 	g_return_val_if_fail (GO_IS_LOCALE_SEL (ls), cur_locale_cp);

 	selection = GTK_MENU_ITEM (go_option_menu_get_history (ls->locales));
	locale = g_object_get_data (G_OBJECT (selection), LOCALE_NAME_KEY);
	if (locale) {
		g_free (cur_locale_cp);
		return g_strdup (locale);
	} else {
		return cur_locale_cp;
	}
}


struct cb_find_entry {
	const char *locale;
	gboolean found;
	int i;
	GSList *path;
};

static void
cb_find_entry (GtkMenuItem *w, struct cb_find_entry *cl)
{
	GtkWidget *sub;

	if (cl->found)
		return;

	sub = gtk_menu_item_get_submenu (w);
	if (sub) {
		GSList *tmp = cl->path = g_slist_prepend (cl->path, GINT_TO_POINTER (cl->i));
		cl->i = 0;

		gtk_container_foreach (GTK_CONTAINER (sub), (GtkCallback)cb_find_entry, cl);
		if (cl->found)
			return;

		cl->i = GPOINTER_TO_INT (cl->path->data);
		cl->path = cl->path->next;
		g_slist_free_1 (tmp);
	} else {
		const char *this_locale =
			g_object_get_data (G_OBJECT (w), LOCALE_NAME_KEY);
		if (this_locale && strcmp (this_locale, cl->locale) == 0) {
			cl->found = TRUE;
			cl->path = g_slist_prepend (cl->path, GINT_TO_POINTER (cl->i));
			cl->path = g_slist_reverse (cl->path);
			return;
		}
	}
	cl->i++;
}

gboolean
go_locale_sel_set_locale (GOLocaleSel *ls, const char *locale)
{
	struct cb_find_entry cl;
	LocaleInfo const *ci;

	g_return_val_if_fail (GO_IS_LOCALE_SEL (ls), FALSE);
	g_return_val_if_fail (locale != NULL, FALSE);

	ci = g_hash_table_lookup (locale_hash, locale);
	if (!ci)
		return FALSE;

	locale = ci->actual_locale;
	if (!locale)
		return FALSE;

	cl.locale = locale;
	cl.found = FALSE;
	cl.i = 0;
	cl.path = NULL;

	gtk_container_foreach (GTK_CONTAINER (ls->locales_menu),
			       (GtkCallback)cb_find_entry,
			       &cl);
	if (!cl.found)
		return FALSE;

	go_option_menu_set_history (ls->locales, cl.path);
	g_slist_free (cl.path);

	return TRUE;
}


void
go_locale_sel_set_sensitive (GOLocaleSel *ls, gboolean sensitive)
{
	g_return_if_fail (GO_IS_LOCALE_SEL (ls));

	gtk_widget_set_sensitive (GTK_WIDGET (ls->locales), sensitive);
}

static void
ls_set_property (GObject      *object,
		 guint         prop_id,
		 const GValue *value,
		 GParamSpec   *pspec)
{
#if 0
	GOLocaleSel *ls = GO_LOCALE_SEL (object);
#endif

	switch (prop_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
ls_get_property (GObject     *object,
		 guint        prop_id,
		 GValue      *value,
		 GParamSpec  *pspec)
{
#if 0
	GOLocaleSel *ls = GO_LOCALE_SEL (object);
#endif

	switch (prop_id)
	{
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
