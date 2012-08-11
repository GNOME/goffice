#ifndef GO_CONF_H
# define GO_CONF_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GOConfNode GOConfNode;

void _go_conf_init (void);
void _go_conf_shutdown (void);

GType    go_conf_node_get_type (void);
GOConfNode * go_conf_get_node       (GOConfNode *parent, gchar const *key);
void	 go_conf_free_node	    (GOConfNode *node);

gboolean go_conf_get_bool	(GOConfNode *node, gchar const *key);
gint	 go_conf_get_int	(GOConfNode *node, gchar const *key);
gdouble	 go_conf_get_double	(GOConfNode *node, gchar const *key);
gchar	*go_conf_get_string	(GOConfNode *node, gchar const *key);
GSList	*go_conf_get_str_list	(GOConfNode *node, gchar const *key);
gchar	*go_conf_get_enum_as_str(GOConfNode *node, gchar const *key);

gboolean go_conf_load_bool	(GOConfNode *node, gchar const *key, gboolean default_val);
gint	 go_conf_load_int	(GOConfNode *node, gchar const *key, gint minima, gint maxima, gint default_val);
gdouble	 go_conf_load_double	(GOConfNode *node, gchar const *key, gdouble minima, gdouble maxima, gdouble default_val);
gchar	*go_conf_load_string	(GOConfNode *node, gchar const *key);
GSList	*go_conf_load_str_list	(GOConfNode *node, gchar const *key);
int	 go_conf_load_enum	(GOConfNode *node, gchar const *key, GType t, int default_val);

void	 go_conf_set_bool	(GOConfNode *node, gchar const *key, gboolean val);
void	 go_conf_set_int	(GOConfNode *node, gchar const *key, gint val);
void	 go_conf_set_double	(GOConfNode *node, gchar const *key, gdouble val);
void	 go_conf_set_string	(GOConfNode *node, gchar const *key, gchar const *str);
void	 go_conf_set_str_list	(GOConfNode *node, gchar const *key, GSList *list);
void	 go_conf_set_enum	(GOConfNode *node, gchar const *key, GType t, gint val);

void	 go_conf_sync		(GOConfNode *node);

typedef void (*GOConfMonitorFunc) (GOConfNode *node, gchar const *key, gpointer data);
void	 go_conf_remove_monitor	(guint monitor_id);
guint	 go_conf_add_monitor	(GOConfNode *node, gchar const *key,
				 GOConfMonitorFunc monitor, gpointer data);

G_END_DECLS

#endif /* GO_CONF_H */
