
// Import some libraries
#include "stdio.h"    // Standard IO
#include "fxlib.h"    // CASIO fx-9860G SDK
#include "dispbios.h" // Display BIOS
#include "timer.h"    // Timer

#pragma region Constants

// true/false since it's 2018
const char true  = 1;           // True and false aren't assigned, weird.
const char false = 0;

// Paddle properties
const char PADDLE_SIZE  =  14; // The size of the paddle
const char PADDLE_THICK =   2; // The thickness of the paddle
const char PADDLE_STEP  =   2; // How many pixels it moves each time
const char PADDLE_LIMIT =  24; // How far the paddle can go

// Display properties
const char DISPLAY_WIDTH    = 128;
const char DISPLAY_HEIGHT   =  64;
const char DISPLAY_CENTER_X =  64; // 128 / 2
const char DISPLAY_CENTER_Y =  32; //  64 / 2

// Scoreboard properties
const char SCOREBOARD_HEIGHT = 11;

// Arena properties
const char ARENA_HEIGHT   = 53;  // DISPLAY_HEIGHT - SCOREBOARD_HEIGHT
const char ARENA_CENTER_Y = 37;  // ARENA_HEIGHT / 2 + SCOREBOARD_HEIGHT

#pragma endregion

#pragma region Variables

// Ball
signed   char ball_x         = 64; // Location
signed   char ball_y         = 37;
unsigned char ball_direction =  1; // Direction. The 2s column is x, units y.

// Player 1
unsigned char player1_score  = 0;
signed   char player1_offset = 0; // Player 1 paddle location (from center)

// Player 2
unsigned char player2_score  = 0;  // Same for P2
signed   char player2_offset = 0;

char paused = 0;

#pragma endregion

void AppQuit()
{
    KillTimer(ID_USER_TIMER1);
    KillTimer(ID_USER_TIMER2);
    KillTimer(ID_USER_TIMER3);
    KillTimer(ID_USER_TIMER4);

    return;
}

#pragma region Drawing functions

/** Draws the scoreboard */
void DrawScoreboard()
{
    char i;
    unsigned char p1_score[4];  // sentaro21: [3] is short. Increase it to [4].
    unsigned char p2_score[4];  // tgr:       Make space for NUL as this is a null-term string (I guess)

    // Background
    for (i = 0; i < 11; i++)
        Bdisp_DrawLineVRAM(0, i, 127, i);

    // A line down to separate the sides
    Bdisp_ClearLineVRAM(64,  0, 64, 11);
    
    // Format int as strings
    sprintf(p1_score, "%03d", player1_score);
    sprintf(p2_score, "%03d", player2_score);

    // Player Scores
    PrintXY((64 - 18) - 4, 2, p1_score, true);
    PrintXY( 64       + 4, 2, p2_score, true);
}

/** Draws the paddles */
void DrawPaddles()
{
    char i;

    // Draw P1
    for (i = 0; i < PADDLE_THICK; i++)
    {
        // Draw the line
        Bdisp_DrawLineVRAM(6 + i, 37 - PADDLE_SIZE / 2 + player1_offset,
                           6 + i, 37 + PADDLE_SIZE / 2 + player1_offset);
    }
                       
    // Draw P2
    for (i = 0; i < PADDLE_THICK; i++)
    {
        // Draw the line
        Bdisp_DrawLineVRAM(121 - i, 37 - PADDLE_SIZE / 2 + player2_offset,
                           121 - i, 37 + PADDLE_SIZE / 2 + player2_offset);
    }
}

/** Draws the ball */
void DrawBall()
{
    char i;

    Bdisp_SetPoint_VRAM(ball_x,     ball_y    , 1);
    Bdisp_SetPoint_VRAM(ball_x - 1, ball_y    , 1);
    Bdisp_SetPoint_VRAM(ball_x + 1, ball_y    , 1);
    Bdisp_SetPoint_VRAM(ball_x,     ball_y + 1, 1);
    Bdisp_SetPoint_VRAM(ball_x,     ball_y - 1, 1);
}

#pragma endregion

#pragma region Timer functions

/** Timer #1: Moves ball and handles collision */
void MoveBall()
{
    // Variables to be used
    char hit_top, hit_btm;

    // Move the ball in the right direction
    switch (ball_direction)
    {
        case 0:
            ball_x--;
            ball_y--;
            break;
    
        case 1:
            ball_x--;
            ball_y++;
            break;
    
        case 2:
            ball_x++;
            ball_y--;
            break;
    
        case 3:
            ball_x++;
            ball_y++;
            break;

        default:
            break;
    }

    // Bounce the ball if it's on the walls
    if (ball_y < 11 || ball_y > 63)
    {
        ball_direction ^= 1; // Flip direction on the y axis
    }

    // Bounce the ball if it's on the paddles
    if (ball_x < 6 + PADDLE_THICK || ball_x > 121 - PADDLE_THICK)
    {
        // Get the paddle values
        if (ball_x < DISPLAY_CENTER_X)
        {
            hit_top = 37 - PADDLE_SIZE / 2 + player1_offset;
            hit_btm = 37 + PADDLE_SIZE / 2 + player1_offset;
        }
        else
        {
            hit_top = 37 - PADDLE_SIZE / 2 + player2_offset;
            hit_btm = 37 + PADDLE_SIZE / 2 + player2_offset;
        }

        // Check if it's hitting the paddle
        if (ball_y > hit_top && ball_y < hit_btm)
            ball_direction ^= 2; // It hit. Flip x axis
        else
        {
            if (ball_x < DISPLAY_CENTER_X)
            {
                player2_score++;
                ball_x = 32;
            }
            else
            {
                player1_score++;
                ball_x = DISPLAY_WIDTH - 32;
            }

            ball_y = ARENA_CENTER_Y;
            ball_direction ^= 2;
        }
    }
}

/** Timer #2: Renders screen */
void RenderScreen()
{
    // Clear current screen
    Bdisp_AllClr_VRAM();
        
    // Draw the different parts of the screen
    DrawScoreboard();
    DrawPaddles();
    DrawBall();

    // Move VRAM to DD
    Bdisp_PutDisp_DD();
}

/** Timer #3: Player 2 Control */
void MoveP2()
{
    signed char ball_relative; // Ball relative to center
    
    // Make ball_y easier to deal with 
    ball_relative = ball_y - ARENA_CENTER_Y;

    // Moves P2 towards ball
    if (player2_offset > ball_relative && player2_offset > 0 - PADDLE_LIMIT)
        player2_offset -= 1;
    else if (player2_offset < ball_relative && player2_offset < PADDLE_LIMIT)
        player2_offset += 1;
}

/** Timer #3: Kills and creates a new timer for Player 2 Control */
void MoveP2_Advance()
{
    KillTimer(ID_USER_TIMER3);
    SetTimer(ID_USER_TIMER3, 50 / (player1_score + 1), MoveP2);
}

/** Timer #4: Key handling */
void KeyHandler()
{
    // sentaro21: IsKeyDown doesn't work as SH4 calcuators
    // if (IsKeyDown(KEY_CTRL_DOWN))
    // {
    //     if (player1_offset < PADDLE_LIMIT)
    //     player1_offset += PADDLE_STEP;
    // }
    
    // else if (IsKeyDown(KEY_CTRL_UP))
    // {
    //     if (player1_offset > 0 - PADDLE_LIMIT)
    //     player1_offset -= PADDLE_STEP;
    // }

    // else if (IsKeyDown(KEY_CTRL_MENU))
    // {
    //     AppQuit();
    // }
}

/** Timer #5: Unused */

#pragma endregion

/** Add-in entry point
 * 
 * Parameters: app_mode - Is launched from the main menu
 *             strip_no - Strip number from eActivity
 * 
 * Returns:    NOT error
 */
int AddIn_main(int app_mode, unsigned short strip_no)
{
    unsigned int i, j; // Iteration variables
    unsigned char str[3];
    unsigned int key;  // Keyboard input

    // Clear out the display
    Bdisp_AllClr_DDVRAM();

    // Draw the scoreboard
    DrawScoreboard();

    // Set up timers for rendering and ball
    SetTimer(ID_USER_TIMER1, 25, RenderScreen);
    SetTimer(ID_USER_TIMER2, 25, MoveBall);
    MoveP2_Advance(); // This function will start the timer for us
    // SetTimer(ID_USER_TIMER4, 25, KeyHandler);

    // Set quit handler
    SetQuitHandler(AppQuit);

    // The main thread manages the keys
    while (true)
    {
        int keyResp = GetKeyWait(KEYWAIT_HALTON_TIMEROFF, 0, 0, &key);
    
        if (keyResp == KEYREP_KEYEVENT)
        {
            if (key == KEY_CTRL_DOWN)
            {
                if (player1_offset < PADDLE_LIMIT)
                player1_offset += PADDLE_STEP;
            }
            
            else if (key == KEY_CTRL_UP)
            {
                if (player1_offset > 0 - PADDLE_LIMIT)
                player1_offset -= PADDLE_STEP;
            }

            else if (key == KEY_CTRL_MENU)
            {
                AppQuit();
            }
        }
    }

    // Good job on somehow breaking the loop. Cleanup time.
    AppQuit();

    // At the end, return application status
    return true;
}

#pragma region Do not touch

// Source code from here down should not be changed according to the
// CASIO SDK sample project.

#pragma section _BR_Size
unsigned long BR_Size;
#pragma section

#pragma section _TOP

//****************************************************************************
//  InitializeSystem
//
//  param   :   isAppli   : 1 = Application / 0 = eActivity
//              OptionNum : Option Number (only eActivity)
//
//  retval  :   1 = No error / 0 = Error
//
//****************************************************************************
int InitializeSystem(int isAppli, unsigned short OptionNum)
{
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}

#pragma section

#pragma endregion
