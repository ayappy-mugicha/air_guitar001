#ifndef CHORDS_H
#define CHORDS_H

void chordsBegin();
void chordsAllOpen();
void chordsPress(int chord_index);
bool chordsHandlePayload(const char *payload);

#endif
