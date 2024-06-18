// gcc command
// gcc -o i2phone_button i2phone_button.c -pthread `pkg-config --cflags --libs gtk+-3.0`

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>

#define BUFFER_SIZE 4096

int running = 1;
int client_socket;
float scale_value = 1.0; // デフォルトのスケール値

void end_call(GtkWidget *widget, gpointer data) {
    running = 0;
}

void scale_changed(GtkRange *range, gpointer data) {
    scale_value = gtk_range_get_value(range) / 100.0; // スケールバーの値を0.0から1.0の範囲に変換
}

void* network_thread(void* arg) {
    FILE *fp_rec;
    char *cmd_rec = "rec -t raw -b 16 -c 1 -e s -r 48000 - ";

    FILE *fp_ply;
    char *cmd_ply = "play -t raw -b 16 -c 1 -e s -r 48000 - ";
    if (client_socket != -1) {
        fp_rec = popen(cmd_rec, "r");
        fp_ply = popen(cmd_ply, "w");
    }

    char DATA_REC[BUFFER_SIZE];
    char DATA_PLY[BUFFER_SIZE];

    while (running) {
        int n_rec = fread(DATA_REC, 1, BUFFER_SIZE, fp_rec);
        if (n_rec == -1) {
            perror("read");
            exit(1);
        }
        if (n_rec == 0) {
            break;
        }
        send(client_socket, DATA_REC, n_rec, 0);

        int n_ply = recv(client_socket, DATA_PLY, BUFFER_SIZE, 0);
        if (n_ply == -1) {
            perror("recv");
            exit(1);
        }
        if (n_ply == 0) {
            break;
        }

        // スケール値に応じて再生データを調整
        for (int i = 0; i < n_ply; ++i) {
            DATA_PLY[i] *= scale_value;
        }

        fwrite(DATA_PLY, 1, n_ply, fp_ply);
    }

    close(client_socket);
    pclose(fp_rec);
    pclose(fp_ply);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "My I3 phone"); // ウィンドウのタイトルを設定

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // CSSの読み込みと適用
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_css_provider_load_from_data(provider,
                                    "window { background-color: #2c3e50; color: #ecf0f1; } "
                                    "button { background-color: #e74c3c; color: #ffffff; } " // End Callボタンを赤色に設定
                                    "scale trough { background-color: #34495e; } "
                                    "scale slider { background-color: #2980b9; } ",
                                    -1, NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *label_title = gtk_label_new("My I3 phone");
    gtk_widget_set_halign(label_title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), label_title, FALSE, FALSE, 10);

    if (argc == 2) {
        GtkWidget *label_mode = gtk_label_new("server");
        gtk_widget_set_halign(label_mode, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(vbox), label_mode, FALSE, FALSE, 10);
    } else if (argc == 3) {
        GtkWidget *label_mode = gtk_label_new("client");
        gtk_widget_set_halign(label_mode, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(vbox), label_mode, FALSE, FALSE, 10);
    }

    GtkWidget *label_volume = gtk_label_new("<b>Volume</b>");
    gtk_label_set_use_markup(GTK_LABEL(label_volume), TRUE);
    gtk_widget_set_halign(label_volume, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), label_volume, FALSE, FALSE, 10);

    GtkWidget *button_end_call = gtk_button_new_with_label("End Call");
    gtk_widget_set_name(button_end_call, "end-call-button"); // CSSでスタイルを適用するために名前を設定
    g_signal_connect(button_end_call, "clicked", G_CALLBACK(end_call), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button_end_call, FALSE, FALSE, 10);

    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(scale), 100); // 初期値を100に設定
    g_signal_connect(scale, "value-changed", G_CALLBACK(scale_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 10);

    gtk_widget_show_all(window);

    if (argc == 2) {
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(atoi(argv[1]));
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));

        listen(server_socket, 10);

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);
        close(server_socket);

    } else {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        if (inet_aton(argv[1], &addr.sin_addr) == 0) {
            perror("inet_aton");
            exit(1);
        }
        addr.sin_port = htons(atoi(argv[2]));
        int ret = connect(client_socket, (struct sockaddr *)&addr, sizeof(addr));
        if (ret == -1) {
            perror("connect");
            exit(1);
        }
    }

    pthread_t net_thread;
    pthread_create(&net_thread, NULL, network_thread, NULL);

    gtk_main();

    running = 0;
    pthread_join(net_thread, NULL);

    return 0;
}
