#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include "SDL_thread.h"
#include "constants.h"
#include <SDL_audio.h>
#include <SDL_ttf.h>
#include <stdlib.h>
#include <time.h>


int score = 0;
bool showStartButton = true; // Declaración y definición inicial
bool isGameStarted = false; // Variable para controlar si el juego ha comenzado
TTF_Font* font; // fuente para nuestro texto
SDL_Texture* buttonTextTexture; //Textura para el texto de nuestro boton iniciar
SDL_Color textColor = { 200, 100, 50 }; // Color del texto

SDL_Thread* audioThread;
SDL_Thread* audioThreadStart;
SDL_Thread* audioThreadOver;
SDL_Thread* audioThreadBonus;
SDL_Thread* audioThreadPaddle;
SDL_Thread* audioThreadWall;
SDL_Texture* scoreTexture = NULL;
SDL_sem* audioSemaphore;
const char* sonido = "";

// Función para crear un texto
SDL_Texture* createTextTexture(const char* text, TTF_Font* font, SDL_Renderer* renderer) {
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    return textTexture;
}

void play_audio(const char* sonido) {
    // Esperamos en el semáforo, lo que bloquea hasta que el semáforo se libere
    SDL_SemWait(audioSemaphore);

    // Aquí iría el código para cargar y reproducir el sonido
    // Por ejemplo:
    SDL_RWops* audioFile = SDL_RWFromFile(sonido, "rb");
    if (audioFile != NULL) {
        // Supongamos que tienes una función para reproducir el sonido
        // play_sound(audioFile);  // Reproducir el sonido
    }

    // Señalizamos (liberamos) el semáforo una vez que el sonido ha terminado
    SDL_SemPost(audioSemaphore);
}

typedef struct {
    Uint8* audioData; // Pointer to audio data    
    Uint32 audioLength; // Length of audio data in bytes    
    Uint32 audioPosition; // Current position in audio data    
    SDL_bool audioFinished; //flag set to True whether audio playback has finished
} AudioContext;

void AudioCallback(void* userdata, Uint8* stream, int len) {
    AudioContext* audioContext = (AudioContext*)userdata;

    if (audioContext->audioPosition >= audioContext->audioLength) {
        audioContext->audioFinished = SDL_TRUE;
        return;
    }

    // Calculate the amount of data to copy to the stream    
    int remainingBytes = audioContext->audioLength - audioContext->audioPosition;
    int bytesToCopy = (len < remainingBytes) ? len : remainingBytes;

    // Copy audio data to the stream    
    SDL_memcpy(stream, audioContext->audioData + audioContext->audioPosition, bytesToCopy);

    // Update the audio position    
    audioContext->audioPosition += bytesToCopy;
}
//Esta es una función de devolución de llamada de audio que se utiliza para proporcionar datos de audio al sistema de audio SDL


typedef struct {
    const char* filename;
    SDL_AudioSpec audioSpec;
    SDL_AudioDeviceID audioDevice;
    Uint8* audioData;
    Uint32 audioLength;
} SoundInfo;

SoundInfo sound_start = { "mix.wav", {}, 0, nullptr, 0 };
SoundInfo sound_wall = { "tap.wav", {}, 0, nullptr, 0 };
SoundInfo sound_paddle = { "cartoon.wav", {}, 0, nullptr, 0 };
SoundInfo sound_dup = { "dup.wav", {}, 0, nullptr, 0 };
SoundInfo sound_over = { "reb.wav", {}, 0, nullptr, 0 };


int play_audio(void* data) {
    // Se obtiene la información del sonido desde el parámetro 'data'
    SoundInfo* soundInfo = static_cast<SoundInfo*>(data);

    // Espera a que el semáforo esté disponible antes de proceder
    SDL_SemWait(audioSemaphore);

    // Carga el archivo WAV y configura el dispositivo de audio
    if (SDL_LoadWAV(soundInfo->filename, &soundInfo->audioSpec, &soundInfo->audioData, &soundInfo->audioLength) != NULL) {
        SDL_PauseAudioDevice(soundInfo->audioDevice, 0); // Inicia la reproducción de audio
    }
    else {
        // Imprime un mensaje de error si no se puede cargar el archivo WAV
        printf("Unable to load WAV file: %s\n", SDL_GetError());
    }

    // Configuración estática para la inicialización del dispositivo de audio
    static uint8_t isaudioDeviceInit = 0;
    static SDL_AudioSpec audioSpec;
    static SDL_AudioDeviceID audioDevice = 0;
    static AudioContext audioContext;

    // Inicialización del dispositivo de audio si aún no se ha hecho
    if (isaudioDeviceInit == 0) {
        audioSpec.freq = 44100;
        audioSpec.format = AUDIO_S16SYS;
        audioSpec.channels = 1;
        audioSpec.samples = 2048;

        audioSpec.callback = AudioCallback;
        audioSpec.userdata = &audioContext;

        audioDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
        if (audioDevice == 0) {
            // Imprime un mensaje de error si no se puede abrir el dispositivo de audio
            printf("Unable to open audio device: %s\n", SDL_GetError());
            return 0;
        }
        isaudioDeviceInit = 1;
    }

    // Configuración inicial para el contexto de audio
    audioContext.audioPosition = 0;
    audioContext.audioFinished = SDL_FALSE;

    // Carga el archivo WAV para la reproducción de audio
    if (SDL_LoadWAV(sonido, &audioSpec, &audioContext.audioData, &audioContext.audioLength) != NULL) {
        SDL_PauseAudioDevice(audioDevice, 0); // Inicia la reproducción de audio
    }
    else {
        // Imprime un mensaje de error si no se puede cargar el archivo WAV
        printf("Unable to load WAV file: %s\n", SDL_GetError());
    }

    // Espera hasta que la reproducción de audio haya terminado
    while (audioContext.audioFinished != SDL_TRUE) {
        SDL_Delay(0.1); // Delay al iniciar el juego
    }

    // Limpia los recursos del audio después de su uso
    SDL_CloseAudioDevice(soundInfo->audioDevice);
    SDL_FreeWAV(soundInfo->audioData);

    // Libera el semáforo después de que se hayan liberado los recursos
    SDL_SemPost(audioSemaphore);

    // Retorna 0 indicando que la función se ejecutó sin errores
    return 0;
}
//Esta función se encarga de la reproducción de audio y se ejecuta en un hilo separado para .


int game_is_running = false;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
int last_frame_time = 0;

struct game_object {
    float x;
    float y;
    float width;
    float height;
    float vel_x;
    float vel_y;
    int score;
    bool damageSquareConsumed;
    int type;
} ball, paddle, button, damageSquare;


// Esta función verifica si se hizo clic en el botón de inicio utilizando las coordenadas del mouse para iniciar el juego.
bool isMouseClickInButton(int mouseX, int mouseY) {
    return (mouseX >= button.x && mouseX <= button.x + button.width &&
        mouseY >= button.y && mouseY <= button.y + button.height);
}

int initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }
    window = SDL_CreateWindow(
        NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_BORDERLESS
    );
    if (!window) {
        fprintf(stderr, "Error creating SDL Window.\n");
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "Error creating SDL Renderer.\n");
        return false;
    }
    // Inicializar SDL2_ttf
    if (TTF_Init() == -1) {
        fprintf(stderr, "Error initializing SDL2_ttf: %s\n", TTF_GetError());
        return false;
    }
    audioSemaphore = SDL_CreateSemaphore(1);
    return true;
}
SDL_Texture* render_text(const char* text, TTF_Font* font, SDL_Color color)
{
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text, color);
    if (text_surface == NULL)
    {
        fprintf(stderr, "Error rendering text: %s\n", TTF_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (texture == NULL)
    {
        fprintf(stderr, "Error creating texture from surface: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(text_surface);

    return texture;
}

void update_score_text() {
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %d", score);

    SDL_Color color = { 255, 255, 255, 255 };

    // Si ya existe una textura de puntaje, destrúyela
    if (scoreTexture != NULL) {
        SDL_DestroyTexture(scoreTexture);
    }

    // Renderiza la nueva textura del puntaje
    scoreTexture = render_text(score_text, font, color);
}
void render_score(void) {
    // Define la posición del puntaje en la pantalla
    SDL_Rect scoreRect = { 10, 10, 200, 40 };

    // Renderiza la textura del puntaje en la pantalla
    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
}

void generateRandomPosition(float* x, float* y) {
    // Establece un rango en la parte superior de la pantalla para que aparezca el cuadrado
    int topRange = 450; //aqui se ajusta el rango de comestible

    // Generador de posiciones aleatorias dentro del rango superior
    *x = (float)(rand() % (WINDOW_WIDTH - (int)damageSquare.width));
    *y = (float)(rand() % topRange);
}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type) {
    case SDL_QUIT:
        game_is_running = false;
        break;
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE)
            game_is_running = false;
        if (event.key.keysym.sym == SDLK_LEFT)
            paddle.vel_x = -400;
        if (event.key.keysym.sym == SDLK_RIGHT) {
            paddle.vel_x = +400;
        }
        break;
    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_LEFT)
            paddle.vel_x = 0;
        if (event.key.keysym.sym == SDLK_RIGHT)
            paddle.vel_x = 0;


    break;
    }

}

SDL_Texture* createTextTexture(const char* text, TTF_Font* font, SDL_Renderer* renderer, SDL_Color textColor, SDL_Color backgroundColor) {
    // Crea una superficie con el color de fondo
    SDL_Surface* textSurface = TTF_RenderText_Shaded(font, text, textColor, backgroundColor);

    if (textSurface == NULL) {
        fprintf(stderr, "Error creating text surface: %s\n", TTF_GetError());
        return NULL;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    return textTexture;
}


void setup(void)
{
    // Initialize values for the the ball object    
    ball.width = 15;
    ball.height = 15;
    ball.x = 20;
    ball.y = 20;
    ball.vel_x = 300;
    ball.vel_y = 300;

    damageSquare.damageSquareConsumed = false;
    damageSquare.width = 100; // Ancho del cuadrado
    damageSquare.height = 100;  // Alto del cuadrado
    damageSquare.x = 200; // Posición x del cuadrado
    damageSquare.y = 200; // Posición y del cuadrado
    // ... otras propiedades del cuadrado si es necesario


    // Initialize the values for the paddle object    
    paddle.width = 100;
    paddle.height = 20;
    paddle.x = (WINDOW_WIDTH / 2) - (paddle.width / 2);
    paddle.y = WINDOW_HEIGHT - 40;
    paddle.vel_x = 0;
    paddle.vel_y = 0;

    SDL_Color buttonColor = { 255, 247, 0 };  // Amarillo
    SDL_Color buttonBackgroundColor = { 128, 0, 128 };  // Morado
    // Crea el botón de inicio
    button.width = 350; // Ancho del botón
    button.height = 150;  // Alto del botón
    button.x = (WINDOW_WIDTH / 2) - (button.width / 2);
    button.y = (WINDOW_HEIGHT / 2);
    buttonTextTexture = createTextTexture("INICIAR JUEGO", font, renderer, buttonColor, buttonBackgroundColor);  
}


void update(void)
{
    // Calculate how much we have to wait until we reach the target frame time    
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);

    // Only delay if we are too fast to update this frame    
    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
        SDL_Delay(time_to_wait);

    // Get a delta time factor converted to seconds to be used to update my objects    
    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0;

    // Store the milliseconds of the current frame    
    last_frame_time = SDL_GetTicks();

    // update ball and paddle position    
    ball.x += ball.vel_x * delta_time;
    ball.y += ball.vel_y * delta_time;
    paddle.x += paddle.vel_x * delta_time;
    paddle.y += paddle.vel_y * delta_time;


    if (score >= 40 && damageSquare.damageSquareConsumed) {
        generateRandomPosition(&damageSquare.x, &damageSquare.y);
        damageSquare.damageSquareConsumed = false;
    }

    if (!damageSquare.damageSquareConsumed && ball.y + ball.height >= damageSquare.y && ball.y <= damageSquare.y + damageSquare.height &&
        ball.x + ball.width >= damageSquare.x && ball.x <= damageSquare.x + damageSquare.width) {
        sonido = "dup.wav";
        audioThreadBonus = SDL_CreateThread(&play_audio, "AudioThread", static_cast<void*>(&sound_dup));
        if (audioThreadBonus == NULL) {
            printf("Unable to create audio thread: %s\n", SDL_GetError());
        }
        // Realiza una acción cuando la pelota colisiona con el cuadrado
        score += 10; // Incrementa el puntaje
        update_score_text();

        // Marca el cuadrado como consumido
        damageSquare.damageSquareConsumed = true;
    }

    // Check for ball collision with the walls 
    if (ball.x <= 0 || ball.x + ball.width >= WINDOW_WIDTH) {
        ball.vel_x = -ball.vel_x;
        sonido = "tap.wav";
        audioThreadWall = SDL_CreateThread(&play_audio, "AudioThread", static_cast<void*>(&sound_wall));
        if (audioThreadWall == NULL) {
            printf("Unable to create audio thread: %s\n", SDL_GetError());
        }
    }

    if (ball.y < 0)
        ball.vel_y = -ball.vel_y;

    // Check for ball collision with the paddle    
    if (ball.y + ball.height >= paddle.y && ball.x + ball.width >= paddle.x && ball.x <= paddle.x + paddle.width) {
        ball.vel_y = -ball.vel_y;

        // Incrementa el puntaje cuando la pelota colisiona con el paddle
        score += 20;
        update_score_text();

        // Inicia un hilo para la reproducción del audio
        sonido = "cartoon.wav";
        audioThreadPaddle = SDL_CreateThread(&play_audio, "AudioThread", static_cast<void*>(&sound_paddle));
        if (audioThreadPaddle == NULL) {
            printf("Unable to create audio thread: %s\n", SDL_GetError());
        }
    }

    // Prevent paddle from moving outside the boundaries of the window    
    if (paddle.x <= 0)
        paddle.x = 0;
    if (paddle.x >= WINDOW_WIDTH - paddle.width)
        paddle.x = WINDOW_WIDTH - paddle.width;

    if (ball.y + ball.height > WINDOW_HEIGHT) {
        ball.x = WINDOW_WIDTH / 2;
        ball.y = 0;
        score = 0; // Reinicia el puntaje a 0 cuando la pelota cae
        update_score_text();
        sonido = "over.wav";
        audioThreadOver = SDL_CreateThread(&play_audio, "AudioThread", static_cast<void*>(&sound_over));
        if (audioThreadOver == NULL) {
            printf("Unable to create audio thread: %s\n", SDL_GetError());
        }
    }
}

void render(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw a rectangle for the ball object    
    SDL_Rect ball_rect = {
        (int)ball.x,
        (int)ball.y,
        (int)ball.width,
        (int)ball.height
    };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &ball_rect);

    // Draw a rectangle for the paddle object    
    SDL_Rect paddle_rect = {
        (int)paddle.x,
        (int)paddle.y,
        (int)paddle.width,
        (int)paddle.height
    };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &paddle_rect);

    if (!damageSquare.damageSquareConsumed) {
        SDL_Rect damageSquareRect = {
            (int)damageSquare.x,
            (int)damageSquare.y,
            (int)damageSquare.width,
            (int)damageSquare.height
        };
        SDL_SetRenderDrawColor(renderer, 50, 0, 80, 0); // Colo del cuadrado 
        SDL_RenderFillRect(renderer, &damageSquareRect);
    }

    // Renderiza el puntaje
    render_score();

    // Solo dibuja el botón cuando showStartButton sea true
    if (showStartButton) {
        SDL_Rect buttonRect = {
            (int)button.x,
            (int)button.y,
            (int)button.width,
            (int)button.height
        };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &buttonRect);
        SDL_RenderCopy(renderer, buttonTextTexture, NULL, &buttonRect);
    }
    SDL_RenderPresent(renderer);
}

void destroy_window(void) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* args[]) {
    game_is_running = initialize_window();
    font = TTF_OpenFont("Fonts/val.ttf", 120); // Ruta a tu fuente y tamaño
    if (!font) {
        fprintf(stderr, "Error loading font: %s\n", TTF_GetError());
        return false;
    }
    setup();

    SDL_WaitThread(audioThreadStart, NULL);
    SDL_WaitThread(audioThreadBonus, NULL);
    SDL_WaitThread(audioThreadOver, NULL);
    SDL_WaitThread(audioThreadWall, NULL);
    SDL_WaitThread(audioThreadPaddle, NULL);
    while (game_is_running) {
        process_input();
        render();

        if (showStartButton) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (isMouseClickInButton(mouseX, mouseY)) 
            {
                showStartButton = false;  // Oculta el botón
                isGameStarted = true;  // Inicia el juego
                // Llama a la función update() después de iniciar el juego
                sonido = "mix.wav";
                audioThreadStart = SDL_CreateThread(&play_audio, "AudioThread", static_cast<void*>(&sound_start));
                if (audioThreadStart == NULL) {
                    printf("Unable to create audio thread: %s\n", SDL_GetError());
                }
            }
        }
        else {
            // El juego ha comenzado, actualiza el juego
            update();
        }
    }
    // Limpiar recursos
    TTF_CloseFont(font);
    SDL_DestroyTexture(buttonTextTexture);
    TTF_Quit();
    destroy_window();
    return 0;
}