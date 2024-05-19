

enum class Status : uint32_t {
    // Success status
    Success,                // Successfully terminated
    Pending,                // Successfully terminated, but it is required to wait end of operation, when async operation is used.
    // Same is warning, but is OK
	Busy,
    Warning,                // Common warning
    Empty,                  // Successfully, but no data.
    Full,                   // Warning, success but no enaught memory
    // Real error
    Error,                  // Error. Use, special Error code if possible.
    Nullptr,                // Pointer to zero
    WrongParameter,         // Wrong argument or pointer   
    Overflow,               // Buffer overflow  (already)
    Underflow,              // Buffer underflow (already)
    SyncronizationError,    // 
    OutOfBoundary,          // Error. Out of data boundary 
    InternalError,          // Module internal error
};


