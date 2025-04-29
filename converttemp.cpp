#include <gtk/gtk.h>
#include <sstream>
#include <iomanip>

GtkWidget *entry, *output, *toggle_button;
bool toFahrenheit = true;

void convert_temp(GtkWidget *, gpointer) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    try {
        double input = std::stod(text);
        double result = toFahrenheit ? (input * 9.0 / 5.0 + 32) : ((input - 32) * 5.0 / 9.0);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << result << (toFahrenheit ? " °F" : " °C");
        gtk_label_set_text(GTK_LABEL(output), ss.str().c_str());
    } catch (...) {
        gtk_label_set_text(GTK_LABEL(output), "Invalid input");
    }
}

void toggle_unit(GtkWidget *, gpointer) {
    toFahrenheit = !toFahrenheit;
    gtk_button_set_label(GTK_BUTTON(toggle_button), toFahrenheit ? "To °F" : "To °C");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Temp Converter");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry, 0, 0, 2, 1);

    output = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), output, 0, 1, 2, 1);

    GtkWidget *convert_button = gtk_button_new_with_label("Convert");
    gtk_grid_attach(GTK_GRID(grid), convert_button, 0, 2, 1, 1);
    g_signal_connect(convert_button, "clicked", G_CALLBACK(convert_temp), NULL);

    toggle_button = gtk_button_new_with_label("To °F");
    gtk_grid_attach(GTK_GRID(grid), toggle_button, 1, 2, 1, 1);
    g_signal_connect(toggle_button, "clicked", G_CALLBACK(toggle_unit), NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
