#include <gtk/gtk.h>
#include <math.h>
#include <dlfcn.h>

typedef double (*DerivativeFunc)(double, double);
typedef double (*PiFunc)(int);

// Global variables for the libraries
DerivativeFunc Derivative = NULL;
PiFunc Pi = NULL;
int current_lib = 0;
void* handle = NULL;

const char* lib_paths[] = {"./libimpl1.so", "./libimpl2.so"};

// Function to switch libraries
void switch_library(GtkWidget* widget, gpointer label) {
    if (handle) {
        dlclose(handle);
    }
    current_lib = 1 - current_lib;

    handle = dlopen(lib_paths[current_lib], RTLD_LAZY);
    if (!handle) {
        g_printerr("Error loading library: %s\n", dlerror());
        gtk_label_set_text(GTK_LABEL(label), "Error loading library");
        return;
    }

    Derivative = (DerivativeFunc)dlsym(handle, "Derivative");
    Pi = (PiFunc)dlsym(handle, "Pi");

    char message[256];
    snprintf(message, sizeof(message), "Switched to Library %d", current_lib + 1);
    gtk_label_set_text(GTK_LABEL(label), message);
}

// Function to calculate derivative
void calculate_derivative(GtkWidget* widget, gpointer result_label) {
    const gchar* A_text = gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(widget), "entry_a")));
    const gchar* deltaX_text = gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(widget), "entry_deltax")));

    double A = atof(A_text);
    double deltaX = atof(deltaX_text);
    double result = Derivative(A, deltaX);

    char result_str[256];
    snprintf(result_str, sizeof(result_str), "Derivative: %.10f", result);
    gtk_label_set_text(GTK_LABEL(result_label), result_str);
}

// Function to calculate Pi
void calculate_pi(GtkWidget* widget, gpointer result_label) {
    const gchar* K_text = gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(widget), "entry_k")));

    int K = atoi(K_text);
    double result = Pi(K);

    char result_str[256];
    snprintf(result_str, sizeof(result_str), "Pi: %.15f", result);
    gtk_label_set_text(GTK_LABEL(result_label), result_str);
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    // Load initial library
    handle = dlopen(lib_paths[current_lib], RTLD_LAZY);
    if (!handle) {
        g_printerr("Error loading library: %s\n", dlerror());
        return 1;
    }
    Derivative = (DerivativeFunc)dlsym(handle, "Derivative");
    Pi = (PiFunc)dlsym(handle, "Pi");

    // Create the main window
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Interactive Mathematical Interface");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Fixed container
    GtkWidget* fixed = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(window), fixed);

    // Title
    GtkWidget* title = gtk_label_new("Interactive Math");
    gtk_widget_set_name(title, "title");
    gtk_fixed_put(GTK_FIXED(fixed), title, 200, 20);

    // Input entries
    GtkWidget* entry_a = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_a), "Enter A");
    gtk_fixed_put(GTK_FIXED(fixed), entry_a, 50, 100);

    GtkWidget* entry_deltax = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_deltax), "Enter deltaX");
    gtk_fixed_put(GTK_FIXED(fixed), entry_deltax, 50, 140);

    // Buttons
    GtkWidget* derivative_button = gtk_button_new_with_label("Calculate Derivative");
    gtk_fixed_put(GTK_FIXED(fixed), derivative_button, 50, 180);

    GtkWidget* pi_button = gtk_button_new_with_label("Calculate Pi");
    gtk_fixed_put(GTK_FIXED(fixed), pi_button, 50, 260);

    GtkWidget* switch_button = gtk_button_new_with_label("Switch Library");
    gtk_fixed_put(GTK_FIXED(fixed), switch_button, 50, 340);

    // Labels for results
    GtkWidget* derivative_result = gtk_label_new("");
    gtk_fixed_put(GTK_FIXED(fixed), derivative_result, 50, 220);

    GtkWidget* pi_result = gtk_label_new("");
    gtk_fixed_put(GTK_FIXED(fixed), pi_result, 50, 300);

    GtkWidget* switch_result = gtk_label_new("");
    gtk_fixed_put(GTK_FIXED(fixed), switch_result, 200, 340);

    // Connect signals
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_object_set_data(G_OBJECT(derivative_button), "entry_a", entry_a);
    g_object_set_data(G_OBJECT(derivative_button), "entry_deltax", entry_deltax);
    g_signal_connect(derivative_button, "clicked", G_CALLBACK(calculate_derivative), derivative_result);

    g_object_set_data(G_OBJECT(pi_button), "entry_k", entry_a);
    g_signal_connect(pi_button, "clicked", G_CALLBACK(calculate_pi), pi_result);

    g_signal_connect(switch_button, "clicked", G_CALLBACK(switch_library), switch_result);

    // Show everything
    gtk_widget_show_all(window);

    gtk_main();
    dlclose(handle);

    return 0;
}
