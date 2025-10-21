#pragma once

namespace hyperv {
    enum class error_code {
        success = 0,
        invalid_parameter = 1,
        // Add more error codes as needed
    };

    void set_last_error(error_code code);
    error_code get_last_error();
}
