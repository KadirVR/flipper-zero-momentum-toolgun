#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

// Menü Modları
const char* modes[] = {"REMOVER", "COPY", "COLOR", "MATERIAL", "WELD"};
#define MODES_COUNT 5

// Uygulama Durumu
typedef struct {
    int mode_index;
    bool is_paste;
} ToolgunState;

// Ekrana Çizim Yapma (Draw Callback)
static void draw_callback(Canvas* canvas, void* ctx) {
    ToolgunState* state = (ToolgunState*)ctx;

    canvas_clear(canvas);
    
    const char* current_mode = modes[state->mode_index];
    // COPY modundaysa ve yukarı basıldıysa PASTE yaz
    if(state->mode_index == 1 && state->is_paste) {
        current_mode = "PASTE";
    }

    // Ekranın tam ortasına modu yazdır
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, current_mode);
    
    // Alt başlık
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignCenter, "[ Tool Gun Mode ]");
}

// Tuş Okuma (Input Callback)
static void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

// Flipper hoparlöründen ses çıkarma fonksiyonları
void play_sound(float freq, int duration_ms) {
    if(furi_hal_speaker_is_mine() || furi_hal_speaker_acquire(100)) {
        furi_hal_speaker_start(freq, 1.0f);
        furi_delay_ms(duration_ms);
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
}

// Ana Uygulama Döngüsü
int32_t toolgun_app(void* p) {
    UNUSED(p);
    
    // Kuyruk ve durum belleği ayırma
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ToolgunState* state = malloc(sizeof(ToolgunState));
    state->mode_index = 0;
    state->is_paste = false;

    // Arayüzü (ViewPort) oluşturma
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, state);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            // Sadece tuşa ilk basıldığında (Short press) işlem yap
            if(event.type == InputTypeShort) {
                switch(event.key) {
                    case InputKeyBack:
                        running = false; // Geri tuşuyla çıkış
                        break;
                    case InputKeyRight:
                        state->mode_index = (state->mode_index + 1) % MODES_COUNT;
                        state->is_paste = false;
                        play_sound(2000.0f, 50); // next.wav alternatifi
                        break;
                    case InputKeyLeft:
                        state->mode_index = (state->mode_index - 1 + MODES_COUNT) % MODES_COUNT;
                        state->is_paste = false;
                        play_sound(2000.0f, 50); // next.wav alternatifi
                        break;
                    case InputKeyUp:
                        if(state->mode_index == 1) { // Sadece COPY modundayken
                            state->is_paste = !state->is_paste;
                            play_sound(1500.0f, 100); // inter.wav alternatifi
                        }
                        break;
                    case InputKeyOk:
                        // %50 şansla iki farklı atış sesi (shoot1 veya shoot2 alternatifi)
                        if (rand() % 2 == 0) {
                            play_sound(800.0f, 150); 
                        } else {
                            play_sound(600.0f, 150); 
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        // Ekranı güncelle
        view_port_update(view_port);
    }

    // Çıkış yaparken belleği temizle
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    free(state);
    furi_record_close(RECORD_GUI);

    return 0;
}