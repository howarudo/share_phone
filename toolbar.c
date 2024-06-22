#include <gtk/gtk.h>
#include <glib.h>

//CheckButtonにチェックが入っているかで、文字を入れるか決める
static void cb_show_text(GtkWidget *widget, gpointer user_data)
{
  if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ){
    gtk_toolbar_set_style(GTK_TOOLBAR(user_data), GTK_TOOLBAR_BOTH);
  }
  else{
    gtk_toolbar_set_style(GTK_TOOLBAR(user_data), GTK_TOOLBAR_ICONS);
  }
}

//ツールアイテムを横に並べる
static void cb_set_horizontal(GtkToggleToolButton *widget, gpointer user_data)
{
  if( gtk_toggle_tool_button_get_active(widget) ){
    GtkOrientable *orientable;
    orientable = GTK_ORIENTABLE(g_object_get_data(G_OBJECT(user_data), "toolbar"));
    gtk_orientable_set_orientation(orientable, GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_size_request(GTK_WIDGET(user_data), 400, -1);
  }
}

//ツールアイテムを縦に並べる
static void cb_set_vertical(GtkToggleToolButton *widget, gpointer user_data)
{
  if( gtk_toggle_tool_button_get_active(widget) ){
    GtkOrientable *orientable;
    orientable = GTK_ORIENTABLE(g_object_get_data(G_OBJECT(user_data), "toolbar"));
    gtk_orientable_set_orientation(orientable, GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_size_request(GTK_WIDGET(user_data), 100, 300);
  }
}

//プログラムを終了する
static void cb_quit(GtkWidget *widget, gpointer user_data)
{
  gtk_main_quit();
}

int main(int argc, char** argv)
{
  GtkWidget *window;
  GtkWidget *toolbar;
  GtkWidget *widget;
  GtkToolItem *item;
  GSList *group;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Toolbar");
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_set_size_request(window, 400, -1);

  //ツールバーを生成する
  toolbar = gtk_toolbar_new();
  gtk_container_add(GTK_CONTAINER(window), toolbar);
  //ツールバーのスタイルをBOTH（文字付き）に設定
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
  g_object_set_data(G_OBJECT(window), "toolbar", (gpointer)toolbar);

  //CheckButtonを生成する
  widget = gtk_check_button_new_with_label("Show text");
  item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(item), widget);
  gtk_tool_item_set_tooltip_text(item, "Toggle wheter show text");
  //CheckButtonにチェックを入れておく
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(cb_show_text), (gpointer)toolbar);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  //Separatorを挿入する
  item = gtk_separator_tool_item_new();
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  //水平表示させるツールボタンを生成する
  item = gtk_radio_tool_button_new_from_stock(NULL, GTK_STOCK_GO_FORWARD);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), "Horizontal");
  //水平表示をアクティブにする
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(item), TRUE);
  //垂直表示用ボタンとのグループ化の準備
  group = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON(item));
  gtk_tool_item_set_tooltip_text(item, "Set the toolbar to horizontal");
  g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(cb_set_horizontal), (gpointer)window);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  //垂直表示させるツールボタンを生成する
  item = gtk_radio_tool_button_new_from_stock(group, GTK_STOCK_GO_DOWN);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), "Vertical");
  gtk_tool_item_set_tooltip_text(item, "Set the toolbar to vertical");
  g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(cb_set_vertical), (gpointer)window);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  //プログラムを終了させるツールボタンを生成する
  widget = gtk_image_new_from_stock(GTK_STOCK_QUIT, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  item = gtk_tool_button_new(widget, "Quit");
  gtk_tool_item_set_tooltip_text(item, "Exit this program");
  g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(cb_quit), (gpointer)window);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

  gtk_widget_show_all(window);
  gtk_main();

  return 0;
}