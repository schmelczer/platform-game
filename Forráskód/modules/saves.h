#ifndef SAVES_H_INCLUDED
#define SAVES_H_INCLUDED

    #include "global.h"
    #include <time.h>
    #include "player.h"
    #include "map.h"

    #define maxNOSaves 5            /**< A save slotok száma */
    #define bufferSize 64000        /**< A mentéshez használ buffer mérete [byte]. */


    extern bool existingSaves[maxNOSaves]; /**< Igaz, ha a save slot üres. */

    void getExistingSaveFiles(time_t ages[], int points[]);
    void deleteSave(int i);
    void saveGame(int currentOptions[]);
    void loadGame(int currentOptions[], int saveID);

#endif // SAVES_H_INCLUDED
