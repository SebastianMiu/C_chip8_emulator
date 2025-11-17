#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


// original CHIP8 display 64x32
#define CHIP8_WIDTH 64
#define CHIP8_HEIGHT 32


/* default colors: background 0x000000
                   pixel 0xffffff
                   */
#define BACKGROUND_COLOR 0x000000
#define PIXEL_COLOR 0xcdf7f6


// beige pixel 0xcdf7f6 green bg 0x4e5e45

#define pixel_size 20 // scale for SDL screen


//states
typedef enum{
    QUIT,
    RUNNING,
    PAUSE,
}emulator_state_t;

typedef struct {
    uint16_t opcode;
    uint16_t NNN;   // constants
    uint8_t NN;
    uint8_t N;
    uint8_t X;  //identifiers
    uint8_t Y;
}instruction_t;

//chip8 machine
typedef struct{
    emulator_state_t state;
    uint8_t ram[4096];  //byte
    bool display[2048]; // 64*32 -- chip8 resolution
    uint16_t stack[12]; //word
    uint16_t *stack_pointer;
    uint8_t V_reg[16];  // registers V0 - VF
    uint16_t I;         // index reg
    bool keyboard[16];
    uint8_t timer1;     // video timer
    uint8_t timer2;     // audio timer
    uint16_t PC;        //program counter
    char* rom_name;
    instruction_t  instruction; //current instr
}chip8_t;


// sdl initialization
bool init_sdl(void) {

    // TO DO: SDL_INIT_TIMER


    Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
    SDL_Init(flags);

    // Check if initialized
    if ((SDL_WasInit(flags) & SDL_INIT_VIDEO) == 0) {
        SDL_Log("Video subsystem does not initialize \n");
        SDL_Log("SDL_Init returned: 0x%x, SDL_WasInit: 0x%x\n", SDL_WasInit(0), SDL_WasInit(flags));
    }
    if ((SDL_WasInit(flags) & SDL_INIT_AUDIO) == 0) {
        SDL_Log("Audio subsystem does not initialize \n");
        SDL_Log("SDL_Init returned: 0x%x, SDL_WasInit: 0x%x\n", SDL_WasInit(0), SDL_WasInit(flags));
    }


    return true;
}


// chip8 initialization
bool init_chip(chip8_t *chip8, char rom_name[]){
    // load font
    const uint8_t font[] = {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    memcpy(&chip8->ram[0x050], font, sizeof(font));

    //load rom

    FILE *rom = fopen(rom_name, "rb");
    if(!rom) {
        SDL_Log("ROM file %s not found", rom_name);
        return false;
    }


    fseek(rom, 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    fseek(rom,0,SEEK_SET);

    const size_t max_size = sizeof chip8->ram - 0x200;
    if(rom_size > max_size){
            SDL_Log("ROM file %s too big", rom_name);
            return false;
    }

    if(fread(&chip8->ram[0x200], rom_size, 1, rom) != 1){
        SDL_Log("Could not read file into chip memory");
        return false;
    }

    fclose(rom);

    // defaults
    chip8->state = RUNNING;
    chip8->PC = 0x200;  // chip8 roms load to 0x200
    chip8->rom_name = rom_name;
    chip8->stack_pointer = &chip8->stack[0];

    return true;
}



/* original keyboard for CHIP8
 * 123C     1234
 * 456D     QWER
 * 789E     ASDF
 * A0BF     ZXCV
*/

void input_handler(chip8_t *chip8) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                chip8->state = QUIT;
                return;

            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        chip8->state = QUIT;
                        puts("QUIT PROGRAM VIA ESC_KEY");
                        return;

                    case SDLK_SPACE:
                        if (chip8->state == RUNNING) {
                            chip8->state = PAUSE;
                            puts("PAUSED");
                        } else chip8->state = RUNNING;

                        break;



                    case SDLK_1:
                        chip8->keyboard[0x1] = true;
                        puts("KEY");
                        break;

                    case SDLK_2:
                        chip8->keyboard[0x2] = true;
                        puts("KEY");
                        break;

                    case SDLK_3:
                        chip8->keyboard[0x3] = true;
                        puts("KEY");
                        break;

                    case SDLK_4:
                        chip8->keyboard[0xC] = true;
                        puts("KEY");
                        break;

                    case SDLK_Q:
                        chip8->keyboard[0x4] = true;
                        puts("KEY");
                        break;

                    case SDLK_W:
                        chip8->keyboard[0x5] = true;
                        puts("KEY");
                        break;

                    case SDLK_E:
                        chip8->keyboard[0x6] = true;
                        puts("KEY");
                        break;

                    case SDLK_R:
                        chip8->keyboard[0xD] = true;
                        puts("KEY");
                        break;

                    case SDLK_A:
                        chip8->keyboard[0x7] = true;
                        puts("KEY");
                        break;

                    case SDLK_S:
                        chip8->keyboard[0x8] = true;
                        puts("KEY");
                        break;

                    case SDLK_D:
                        chip8->keyboard[0x9] = true;
                        puts("KEY");
                        break;

                    case SDLK_F:
                        chip8->keyboard[0xE] = true;
                        puts("KEY");
                        break;

                    case SDLK_Z:
                        chip8->keyboard[0xA] = true;
                        puts("KEY");
                        break;

                    case SDLK_X:
                        chip8->keyboard[0x0] = true;
                        puts("KEY");
                        break;

                    case SDLK_C:
                        chip8->keyboard[0xB] = true;
                        puts("KEY");
                        break;

                    case SDLK_V:
                        chip8->keyboard[0xF] = true;
                        puts("KEY");
                        break;

                    default:
                        break;
                }
                break;



            case SDL_EVENT_KEY_UP:
                switch (event.key.key) {

                    case SDLK_1:
                        chip8->keyboard[0x1] = false;
                        break;

                    case SDLK_2:
                        chip8->keyboard[0x2] = false;
                        break;

                    case SDLK_3:
                        chip8->keyboard[0x3] = false;
                        break;

                    case SDLK_4:
                        chip8->keyboard[0xC] = false;
                        break;

                    case SDLK_Q:
                        chip8->keyboard[0x4] = false;
                        break;

                    case SDLK_W:
                        chip8->keyboard[0x5] = false;
                        break;

                    case SDLK_E:
                        chip8->keyboard[0x6] = false;
                        break;

                    case SDLK_R:
                        chip8->keyboard[0xD] = false;
                        break;

                    case SDLK_A:
                        chip8->keyboard[0x7] = false;
                        break;

                    case SDLK_S:
                        chip8->keyboard[0x8] = false;
                        break;

                    case SDLK_D:
                        chip8->keyboard[0x9] = false;
                        break;

                    case SDLK_F:
                        chip8->keyboard[0xE] = false;
                        break;

                    case SDLK_Z:
                        chip8->keyboard[0xA] = false;
                        break;

                    case SDLK_X:
                        chip8->keyboard[0x0] = false;
                        break;

                    case SDLK_C:
                        chip8->keyboard[0xB] = false;
                        break;

                    case SDLK_V:
                        chip8->keyboard[0xF] = false;
                        break;


                    default:
                        break;
                }
                break;


        }

    }

}


void compute_instruction(chip8_t *chip8) {
    // FETCH
    chip8->instruction.opcode = (chip8->ram[chip8->PC] << 8) | chip8->ram[chip8->PC + 1];
    chip8->PC += 2;

    // DECODE
    uint16_t opcode = chip8->instruction.opcode;
    chip8->instruction.NNN = opcode & 0x0FFF;
    chip8->instruction.NN = opcode & 0x00FF;
    chip8->instruction.N = opcode & 0x000F;
    chip8->instruction.X = (opcode >> 8) & 0x000F;
    chip8->instruction.Y = (opcode >> 4) & 0x000F;

    uint8_t *V = chip8->V_reg;

    // EXECUTE
    switch ((opcode >> 12) & 0x000F) {
        case 0x0:
            switch (chip8->instruction.NN) {
                case 0xE0: // 00E0: CLS - clear screen
                    memset(chip8->display, 0, sizeof(chip8->display));
                    break;
                case 0xEE: // 00EE: RET - return from subroutine
                    chip8->stack_pointer--;
                    chip8->PC = *chip8->stack_pointer;
                    break;
                default:
                    SDL_Log("SYS call 0x%03X ignored", chip8->instruction.NNN);
                    break;
            }
            break;

        case 0x1: // 1NNN: JP addr
            chip8->PC = chip8->instruction.NNN;
            break;

        case 0x2: // 2NNN: CALL addr
            *chip8->stack_pointer++ = chip8->PC;

            chip8->PC = chip8->instruction.NNN;
            break;

        case 0x3: // 3XNN: SE Vx, NN
            if (V[chip8->instruction.X] == chip8->instruction.NN)
                chip8->PC += 2;
            break;

        case 0x4: // 4XNN: SNE Vx, NN
            if (V[chip8->instruction.X] != chip8->instruction.NN)
                chip8->PC += 2;
            break;

        case 0x5: // 5XY0: Skip next if Vx == Vy
            if (chip8->instruction.N == 0 &&
                V[chip8->instruction.X] == V[chip8->instruction.Y])
                chip8->PC += 2;
            break;

        case 0x6: // 6XNN: LD Vx, NN
            V[chip8->instruction.X] = chip8->instruction.NN;
            break;

        case 0x7: // 7XNN: ADD Vx, NN
            V[chip8->instruction.X] += chip8->instruction.NN;
            break;

        case 0x8: { // Arithmetic and bitwise ops
            uint8_t X = chip8->instruction.X;
            uint8_t Y = chip8->instruction.Y;

            switch (chip8->instruction.N) {
                case 0x0:
                    V[X] = V[Y];
                    break;                          // 8XY0: LD
                case 0x1:
                    V[X] |= V[Y];
                    break;                         // 8XY1: OR
                case 0x2:
                    V[X] &= V[Y];
                    break;                         // 8XY2: AND
                case 0x3:
                    V[X] ^= V[Y];
                    break;                         // 8XY3: XOR
                case 0x4: {                                            // 8XY4: ADD
                    uint16_t sum = V[X] + V[Y];
                    V[0xF] = sum > 0xFF;
                    V[X] = sum & 0xFF;
                    break;
                }
                case 0x5:                                              // 8XY5: SUB
                    V[0xF] = V[X] > V[Y];
                    V[X] -= V[Y];
                    break;
                case 0x6:                                              // 8XY6: SHR
                    V[0xF] = V[X] & 0x1;
                    V[X] >>= 1;
                    break;
                case 0x7:                                              // 8XY7: SUBN
                    V[0xF] = V[Y] > V[X];
                    V[X] = V[Y] - V[X];
                    break;
                case 0xE:                                              // 8XYE: SHL
                    V[0xF] = (V[X] & 0x80) >> 7;
                    V[X] <<= 1;
                    break;
                default:
                    SDL_Log("Unknown 0x8 opcode: 0x%04X", opcode);
                    break;
            }
            break;
        }

        case 0x9: // 9XY0: Skip next if Vx != Vy
            if (chip8->instruction.N == 0 &&
                V[chip8->instruction.X] != V[chip8->instruction.Y])
                chip8->PC += 2;
            break;

        case 0xA: // ANNN: LD I, addr
            chip8->I = chip8->instruction.NNN;
            break;

        case 0xB: // BNNN: JP V0 + addr
            chip8->PC = chip8->instruction.NNN + V[0];
            break;

        case 0xC: // CXNN: RND Vx, byte
            V[chip8->instruction.X] = (rand() % 256) & chip8->instruction.NN;
            break;

        case 0xD: { // DXYN: DRW Vx, Vy, N (draw sprite)
            uint8_t x = V[chip8->instruction.X] % CHIP8_WIDTH;
            uint8_t y = V[chip8->instruction.Y] % CHIP8_HEIGHT;
            uint8_t height = opcode & 0x000F;

            V[0xF] = 0; // reset collision flag

            for (int row = 0; row < height; row++) {
                uint8_t sprite_byte = chip8->ram[chip8->I + row];

                for (int col = 0; col < 8; col++) {
                    if (sprite_byte & (0x80 >> col)) {
                        int px = (x + col) % 64;
                        int py = (y + row) % 32;
                        if (px >= CHIP8_WIDTH || py >= CHIP8_HEIGHT) continue;

                        int idx = py * CHIP8_WIDTH + px;
                        if (chip8->display[idx])
                            V[0xF] = 1; // collision


                        chip8->display[idx] ^= 1;
                    }
                }
            }
            break;


        }
            break;


//            // Debug
//            SDL_Log("DRAW @ PC=0x%03X I=0x%03X Vx=0x%02X Vy=0x%02X N=%d",
//                    chip8->PC - 2, chip8->I,
//                    V[chip8->instruction.X], V[chip8->instruction.Y],
//                    chip8->instruction.N);
//
//            for (int row = 0; row < chip8->instruction.N; row++) {
//                uint8_t b = chip8->ram[chip8->I + row];
//                SDL_Log(" sprite[%d] = 0x%02X", row, b);
            //       }




        case 0xE: { // Key operations
            uint8_t X = chip8->instruction.X;
            switch (chip8->instruction.NN) {
                case 0x9E: // EX9E: Skip next if key VX pressed
                    if (chip8->keyboard[V[X]]) chip8->PC += 2;
                    break;
                case 0xA1: // EXA1: Skip next if key VX not pressed
                    if (!chip8->keyboard[V[X]]) chip8->PC += 2;
                    break;
                default:
                    SDL_Log("Unknown 0xE opcode: 0x%04X", opcode);
                    break;
            }
            break;
        }


        case 0xF: {
            switch (chip8->instruction.NN) {
                case 0x0A:
                    //0xF0A: VX = get key
                    bool key_pressed = false;
                    for (uint8_t i = 0; i < sizeof chip8->keyboard; i++) {
                        if (chip8->keyboard[i]) {
                            V[chip8->instruction.X] = i;
                            key_pressed = true;
                            break;
                        }
                    }

                    if (!key_pressed){
                        chip8->PC -= 2;
                        return; //keep same opcode if not pressed
                        }
                    break;


                case 0x1E:
                    // I += VX;
                    chip8->I += V[chip8->instruction.X];
                    break;

                case 0x07:
                    //   VX = delay timer
                    V[chip8->instruction.X] = chip8->timer1;
                    break;

                case 0x15:
                    //  delay timer = VX
                    chip8->timer1 = V[chip8->instruction.X];
                    break;

                case 0x18:
                    //  audio timer = VX
                    chip8->timer2 = V[chip8->instruction.X];
                    break;

                case 0x29:
                    //  set I to sprite location for char in VX
                    chip8->I = 0x050+ V[chip8->instruction.X] * 5;
                    break;


                case 0x33:                                            // FX33: BCD
                    chip8->ram[chip8->I] = V[chip8->instruction.X] / 100;
                    chip8->ram[chip8->I + 1] = (V[chip8->instruction.X] / 10) % 10;
                    chip8->ram[chip8->I + 2] = V[chip8->instruction.X] % 10;
                    break;

                case 0x55:                                             // FX55: Store V0..VX
                    for (int i = 0; i <= chip8->instruction.X; i++)
                        chip8->ram[chip8->I + i] = V[i];
                    break;

                case 0x65:                                             // FX65: Load V0..VX
                    for (int i = 0; i <= chip8->instruction.X; i++)
                        V[i] = chip8->ram[chip8->I + i];
                    break;

                default:

                    break;

            }
            break;


            default:
                SDL_Log("Unknown opcode: 0x%04X at PC=0x%03X", opcode, chip8->PC - 2);
            break;
        }
    }
}



    // debug
//    if ((opcode & 0xF000) == 0xD000)
//        SDL_Log("DRAW instruction at PC=0x%03X I=0x%03X", chip8->PC-2, chip8->I);









int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "not enough files \n" );

    }

    srand((unsigned) time(NULL));


    // declare

    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;




    //  initialize sdl
    if (!init_sdl()) exit(EXIT_FAILURE);






    if (!SDL_CreateWindowAndRenderer("CHIP-8", CHIP8_WIDTH*pixel_size,
                                     CHIP8_HEIGHT*pixel_size, SDL_WINDOW_OPENGL, &win, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create window and renderer: %s", SDL_GetError());

    }


    if (win == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);

    }

    if (renderer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);

    }


    // initialize chip8
    chip8_t chip8 = {0};
    if (!init_chip(&chip8, argv[1])) {
        exit(EXIT_FAILURE);
    }


    SDL_RenderClear(renderer);

    // get rgb background
    uint8_t bg_r = (BACKGROUND_COLOR >> 16) & 0xFF;
    uint8_t bg_g = (BACKGROUND_COLOR >> 8) & 0xFF;
    uint8_t bg_b = BACKGROUND_COLOR & 0xFF;


    //get rgb pixels
    uint8_t pixel_r = (PIXEL_COLOR >> 16) & 0xFF;
    uint8_t pixel_g = (PIXEL_COLOR >> 8) & 0xFF;
    uint8_t pixel_b = PIXEL_COLOR & 0xFF;


    // main loop
    while (chip8.state != QUIT) {

        uint64_t now = SDL_GetTicks();
        uint64_t last_tick = 0;

        if (now - last_tick >= 16) {      // ~60 Hz (1000/60 = 16.666)
            if (chip8.timer1 > 0) chip8.timer1--;
            if (chip8.timer2 > 0) chip8.timer2--;
            last_tick = now;
        }




        input_handler(&chip8);

        if (chip8.state == PAUSE) continue;

        for(int i=0; i<8; i++) {
            compute_instruction(&chip8);
        }

        SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, pixel_r, pixel_g, pixel_b, 255);


        for (int y = 0; y < CHIP8_HEIGHT; y++) {
            for (int x = 0; x < CHIP8_WIDTH; x++) {
                if (chip8.display[y * CHIP8_WIDTH + x]) {
                    SDL_FRect rect = { x * pixel_size, y * pixel_size, pixel_size, pixel_size };
                    SDL_RenderFillRect(renderer, &rect);




                }

            }
        }

        SDL_RenderPresent(renderer);


        //60Hz
        SDL_Delay(1000/60);


        //compute_instruction(&chip8);


    }

    // final clean
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    exit(EXIT_SUCCESS);



    return 0;
}

