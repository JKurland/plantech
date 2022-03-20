namespace pt

// Emitted when handler constructors have all just finished, load things that couldn't be loaded during
// construction.
event ProgramStart {}


// Emitted when the program is about to end, you can still talk to other handlers
// after this event has been emitted and you must stay capable of responding
// to the requests of others. The point is to get yourself in order to be
// destructed, since during destruction you won't be able to make new requests.
// Once all handlers have returned from EVENT(ProgramEnd) the destructors will be called
// and main exits.
event ProgramEnd {}


// Something has requested a quit, this doesn't necessarily mean the program will actually quit
event QuitRequested {
    int exit_status
}