[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/XglgMq0o)
# Documentación del Proyecto
---
## . Variables Globales:
Estas son algunas de las variables que se declaran al principio:


int score = 0;  // Puntaje del jugador.
int game_started = 0;  // Indica si el juego ha comenzado o no.
SDL_Texture *start_button_texture;  // Textura del botón de inicio.
SDL_Texture *score_texture;  // Textura que mostrará el puntaje.
SDL_AudioSpec audio_spec;  // Especificaciones del audio (por ejemplo, frecuencia, tamaño de buffer).
SDL_Thread *audio_thread;  // Hilo para manejar la reproducción de audio.
score: Se utiliza para almacenar el puntaje del jugador.
game_started: Es una variable booleana que indica si el juego ha comenzado o no. Se usa para controlar el flujo del juego.
start_button_texture: Almacena la textura del botón de inicio, que aparece en la pantalla inicial.
score_texture: Es la textura donde se dibujará el puntaje.
audio_spec: Define cómo se deben manejar los datos de audio.
audio_thread: Es un hilo que se utiliza para reproducir efectos de sonido sin bloquear el flujo del juego.
2. Funciones de Texto:
Estas funciones permiten crear y mostrar texto en la pantalla:

c
Copiar código
SDL_Texture* createTextTexture(const char *text, TTF_Font *font, SDL_Color color) {
    // Crea una textura con un texto dado, utilizando una fuente y color específicos.
}

void render_text(SDL_Renderer *renderer, const char *text, int x, int y, TTF_Font *font, SDL_Color color) {
    // Renderiza el texto en el renderer en la posición (x, y).
}
createTextTexture: Esta función recibe el texto que se quiere mostrar, la fuente y el color, y devuelve una textura que puede ser renderizada en la pantalla.
render_text: Utiliza la textura generada por createTextTexture para dibujar el texto en la pantalla en las coordenadas indicadas.
3. Manejo de Audio:
Estas funciones manejan la reproducción de efectos de sonido en el juego.

c
Copiar código
void play_audio(const char *audio_path) {
    // Reproduce un archivo de audio específico (por ejemplo, sonido de la pelota).
}

void AudioCallback(void *userdata, Uint8 *stream, int len) {
    // Callback que SDL llama cuando se necesita enviar datos de audio.
}
play_audio: Esta función recibe la ruta del archivo de audio y lo reproduce. El código está diseñado para ejecutar esto en un hilo separado para no bloquear la lógica del juego.
AudioCallback: Esta función es llamada por SDL cuando se necesita enviar datos de audio al sistema. Asegura que el audio se procese correctamente en tiempo real.
4. Inicialización de la Ventana y Renderer:
Esta función inicializa SDL y crea una ventana para mostrar el juego:

c
Copiar código
void initialize_window(SDL_Window **window, SDL_Renderer **renderer) {
    // Inicializa SDL y crea una ventana y un renderer.
}
initialize_window: Inicializa SDL, crea una ventana y un renderer que se usará para dibujar todos los gráficos del juego.
5. Manejo de Entrada de Usuario:
Esta parte gestiona los eventos del teclado, como mover el paddle o cerrar el juego:

c
Copiar código
void process_input(SDL_Event *e) {
    // Procesa las entradas de teclado (movimiento del paddle, cierre del juego).
}
process_input: Se encarga de gestionar la entrada del usuario (movimientos del paddle usando las teclas izquierda y derecha, y cerrar el juego con la tecla ESC).
6. Lógica del Juego:
Aquí es donde se actualizan los elementos del juego, como la pelota, el paddle y el puntaje:

c
Copiar código
void update() {
    // Lógica para mover la pelota, chequear colisiones y actualizar el puntaje.
}
update: Se encarga de mover la pelota, chequear si colisiona con las paredes o el paddle, y si toca un cuadrado de daño, se ajusta el puntaje y se reproduce un sonido. También se actualiza el puntaje que se mostrará en pantalla.
7. Botón de Inicio:
El código incluye un botón de inicio que permite al jugador comenzar el juego:

c
Copiar código
int isMouseClickInButton(int x, int y, SDL_Rect button_rect) {
    // Verifica si el clic del ratón está dentro de la zona del botón de inicio.
}
isMouseClickInButton: Verifica si el clic del ratón cae dentro de las coordenadas de un botón. Esto es útil para el botón de inicio, que el jugador puede presionar para comenzar el juego.
8. Configuración de Objetos del Juego:
Aquí se configuran los objetos como la pelota y el paddle:

c

void setup() {
    // Inicializa la pelota, el paddle y los cuadrados de daño.
}
setup: Configura las propiedades iniciales de los objetos del juego, como la posición inicial de la pelota y el paddle, y la configuración de los cuadrados de daño (que son obstáculos en el camino de la pelota).
9. Control del Juego:
Dependiendo de si el juego ha comenzado o no, se actualiza la pantalla. Si el juego no ha comenzado, se muestra un botón de inicio:

c
Copiar código
if (!game_started) {
    // Muestra el botón de inicio.
}
Si el juego ha comenzado, se gestionan las actualizaciones de la lógica del juego (movimiento de la pelota, colisiones, puntaje, etc.).

Resumen de la Funcionalidad:
El juego comienza con un botón de inicio. Al presionar este botón, el juego comienza y se inicia el conteo del puntaje.
La pelota se mueve por la pantalla y rebota al chocar con las paredes y el paddle.
El puntaje se actualiza cada vez que la pelota toca el paddle o un cuadrado de daño.
Los sonidos se reproducen de manera asincrónica mediante hilos, lo que evita que el juego se detenga mientras se reproduce un sonido.
Los eventos del teclado permiten mover el paddle y cerrar el juego.
El código gestiona la entrada del usuario, la lógica del juego, la actualización del puntaje y la reproducción de sonidos.
Estudiante: Cristian Usuga, Jhonier Mosquera

Id:  ,369010
---
