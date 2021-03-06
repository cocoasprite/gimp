/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpdatafactory.h"
#include "gimpfilteredcontainer.h"
#include "gimppaintinfo.h"
#include "gimptoolinfo.h"
#include "gimptooloptions.h"
#include "gimptoolpreset.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VISIBLE
};


static void    gimp_tool_info_dispose         (GObject       *object);
static void    gimp_tool_info_finalize        (GObject       *object);
static void    gimp_tool_info_get_property    (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
static void    gimp_tool_info_set_property    (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);
static gchar * gimp_tool_info_get_description (GimpViewable  *viewable,
                                               gchar        **tooltip);


G_DEFINE_TYPE (GimpToolInfo, gimp_tool_info, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_tool_info_parent_class


static void
gimp_tool_info_class_init (GimpToolInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->dispose           = gimp_tool_info_dispose;
  object_class->finalize          = gimp_tool_info_finalize;
  object_class->get_property      = gimp_tool_info_get_property;
  object_class->set_property      = gimp_tool_info_set_property;

  viewable_class->get_description = gimp_tool_info_get_description;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_VISIBLE,
                            "visible",
                            _("Visible"),
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_tool_info_init (GimpToolInfo *tool_info)
{
}

static void
gimp_tool_info_dispose (GObject *object)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  if (tool_info->tool_options)
    {
      g_object_run_dispose (G_OBJECT (tool_info->tool_options));
      g_object_unref (tool_info->tool_options);
      tool_info->tool_options = NULL;
    }

  if (tool_info->presets)
    {
      g_object_unref (tool_info->presets);
      tool_info->presets = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_info_finalize (GObject *object)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  if (tool_info->label)
    {
      g_free (tool_info->label);
      tool_info->label = NULL;
    }
  if (tool_info->tooltip)
    {
      g_free (tool_info->tooltip);
      tool_info->tooltip = NULL;
    }

  if (tool_info->menu_label)
    {
      g_free (tool_info->menu_label);
      tool_info->menu_label = NULL;
    }
  if (tool_info->menu_accel)
    {
      g_free (tool_info->menu_accel);
      tool_info->menu_accel = NULL;
    }

  if (tool_info->help_domain)
    {
      g_free (tool_info->help_domain);
      tool_info->help_domain = NULL;
    }
  if (tool_info->help_id)
    {
      g_free (tool_info->help_id);
      tool_info->help_id = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_info_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value, tool_info->visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_info_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      tool_info->visible = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gchar *
gimp_tool_info_get_description (GimpViewable  *viewable,
                                gchar        **tooltip)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (viewable);

  if (tooltip)
    *tooltip = g_strdup (tool_info->tooltip);

  return g_strdup (tool_info->label);
}

static gboolean
gimp_tool_info_filter_preset (GimpObject *object,
                              gpointer    user_data)
{
  GimpToolPreset *preset    = GIMP_TOOL_PRESET (object);
  GimpToolInfo   *tool_info = user_data;

  return preset->tool_options->tool_info == tool_info;
}

GimpToolInfo *
gimp_tool_info_new (Gimp                *gimp,
                    GType                tool_type,
                    GType                tool_options_type,
                    GimpContextPropMask  context_props,
                    const gchar         *identifier,
                    const gchar         *label,
                    const gchar         *tooltip,
                    const gchar         *menu_label,
                    const gchar         *menu_accel,
                    const gchar         *help_domain,
                    const gchar         *help_id,
                    const gchar         *paint_core_name,
                    const gchar         *icon_name)
{
  GimpPaintInfo *paint_info;
  GimpToolInfo  *tool_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);
  g_return_val_if_fail (tooltip != NULL, NULL);
  g_return_val_if_fail (menu_label != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (paint_core_name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimp->paint_info_list, paint_core_name);

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  tool_info = g_object_new (GIMP_TYPE_TOOL_INFO,
                            "name",      identifier,
                            "icon-name", icon_name,
                            NULL);

  tool_info->gimp              = gimp;
  tool_info->tool_type         = tool_type;
  tool_info->tool_options_type = tool_options_type;
  tool_info->context_props     = context_props;

  tool_info->label             = g_strdup (label);
  tool_info->tooltip           = g_strdup (tooltip);

  tool_info->menu_label        = g_strdup (menu_label);
  tool_info->menu_accel        = g_strdup (menu_accel);

  tool_info->help_domain       = g_strdup (help_domain);
  tool_info->help_id           = g_strdup (help_id);

  tool_info->paint_info        = paint_info;

  if (tool_info->tool_options_type == paint_info->paint_options_type)
    {
      tool_info->tool_options = g_object_ref (paint_info->paint_options);
    }
  else
    {
      tool_info->tool_options = g_object_new (tool_info->tool_options_type,
                                              "gimp", gimp,
                                              "name", identifier,
                                              NULL);
    }

  g_object_set (tool_info->tool_options,
                "tool",      tool_info,
                "tool-info", tool_info, NULL);

  if (tool_info->tool_options_type != GIMP_TYPE_TOOL_OPTIONS)
    {
      GimpContainer *presets;

      presets = gimp_data_factory_get_container (gimp->tool_preset_factory);

      tool_info->presets =
        gimp_filtered_container_new (presets,
                                     gimp_tool_info_filter_preset,
                                     tool_info);
    }

  return tool_info;
}

void
gimp_tool_info_set_standard (Gimp         *gimp,
                             GimpToolInfo *tool_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));

  if (tool_info != gimp->standard_tool_info)
    {
      if (gimp->standard_tool_info)
        g_object_unref (gimp->standard_tool_info);

      gimp->standard_tool_info = tool_info;

      if (gimp->standard_tool_info)
        g_object_ref (gimp->standard_tool_info);
    }
}

GimpToolInfo *
gimp_tool_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_tool_info;
}

GFile *
gimp_tool_info_get_options_file (GimpToolInfo *tool_info,
                                 const gchar  *suffix)
{
  gchar *basename;
  GFile *file;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);

  /* also works for a NULL suffix */
  basename = g_strconcat (gimp_object_get_name (tool_info), suffix, NULL);

  file = gimp_directory_file ("tool-options", basename, NULL);
  g_free (basename);

  return file;
}
