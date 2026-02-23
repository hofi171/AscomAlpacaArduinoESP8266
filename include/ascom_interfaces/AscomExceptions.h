/**
 * @file AscomExceptions.h
 * @brief ASCOM Alpaca Exception Definitions
 * 
 * This file defines exception codes and error handling for ASCOM Alpaca devices.
 * 
 * Reference: https://ascom-standards.org/newdocs/exceptions.html
 */

#ifndef ASCOMEXCEPTIONS_H
#define ASCOMEXCEPTIONS_H

#include <string>
#include <exception>

/**
 * @enum AscomErrorCode
 * @brief ASCOM standard error codes
 */
enum AscomErrorCode {
    // Success
    ASCOM_OK = 0x0,
    
    // ASCOM driver errors (0x400 - 0x4FF)
    ASCOM_ERROR_NOT_IMPLEMENTED = 0x400,
    ASCOM_ERROR_INVALID_VALUE = 0x401,
    ASCOM_ERROR_VALUE_NOT_SET = 0x402,
    ASCOM_ERROR_NOT_CONNECTED = 0x407,
    ASCOM_ERROR_INVALID_WHILE_PARKED = 0x408,
    ASCOM_ERROR_INVALID_WHILE_SLAVED = 0x409,
    ASCOM_ERROR_SETTINGS_PROVIDER = 0x40A,
    ASCOM_ERROR_INVALID_OPERATION = 0x40B,
    ASCOM_ERROR_ACTION_NOT_IMPLEMENTED = 0x40C,
    
    // COM errors (0x80040000 range)
    ASCOM_ERROR_UNSPECIFIED = 0x80040000,
    ASCOM_ERROR_NOT_INITIALIZED = 0x80040001,
    
    // Custom driver errors (0x500 - 0xFFF)
    ASCOM_ERROR_DRIVER_BASE = 0x500
};

/**
 * @class AscomException
 * @brief Base exception class for ASCOM errors
 */
class AscomException : public std::exception {
private:
    AscomErrorCode errorCode;
    std::string message;
    
public:
    /**
     * @brief Constructor
     * @param code ASCOM error code
     * @param msg Error message
     */
    AscomException(AscomErrorCode code, const std::string& msg) 
        : errorCode(code), message(msg) {}
    
    /**
     * @brief Get error code
     * @return ASCOM error code
     */
    AscomErrorCode GetErrorCode() const { return errorCode; }
    
    /**
     * @brief Get error message
     * @return Error message
     */
    const char* what() const noexcept override {
        return message.c_str();
    }
};

/**
 * @class AscomNotImplementedException
 * @brief Exception for unimplemented functionality
 */
class AscomNotImplementedException : public AscomException {
public:
    AscomNotImplementedException(const std::string& msg = "Method not implemented")
        : AscomException(ASCOM_ERROR_NOT_IMPLEMENTED, msg) {}
};

/**
 * @class AscomInvalidValueException
 * @brief Exception for invalid parameter values
 */
class AscomInvalidValueException : public AscomException {
public:
    AscomInvalidValueException(const std::string& msg = "Invalid value")
        : AscomException(ASCOM_ERROR_INVALID_VALUE, msg) {}
};

/**
 * @class AscomNotConnectedException
 * @brief Exception when device is not connected
 */
class AscomNotConnectedException : public AscomException {
public:
    AscomNotConnectedException(const std::string& msg = "Device not connected")
        : AscomException(ASCOM_ERROR_NOT_CONNECTED, msg) {}
};

/**
 * @class AscomInvalidOperationException
 * @brief Exception for invalid operations
 */
class AscomInvalidOperationException : public AscomException {
public:
    AscomInvalidOperationException(const std::string& msg = "Invalid operation")
        : AscomException(ASCOM_ERROR_INVALID_OPERATION, msg) {}
};

/**
 * @class AscomParkedException
 * @brief Exception when operation invalid while parked
 */
class AscomParkedException : public AscomException {
public:
    AscomParkedException(const std::string& msg = "Invalid while parked")
        : AscomException(ASCOM_ERROR_INVALID_WHILE_PARKED, msg) {}
};

/**
 * @class AscomSlavedException
 * @brief Exception when operation invalid while slaved
 */
class AscomSlavedException : public AscomException {
public:
    AscomSlavedException(const std::string& msg = "Invalid while slaved")
        : AscomException(ASCOM_ERROR_INVALID_WHILE_SLAVED, msg) {}
};

#endif // ASCOMEXCEPTIONS_H
