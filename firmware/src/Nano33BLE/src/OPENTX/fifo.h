// -*- coding: utf-8 -*-
/**
 @file      FIFO.hpp
 @brief     Variable Length Ring Buffer
 
 This library provides variable length ring buffering with software FIFO(First-in, First-out).
 This library overwrites when the buffer is full.
 
 @author    T.Kawamura
 @version   2.1
 @date      2016-11-20  T.Kawamura  Written for AVR.
 @date      2017-03-28  T.Kawamura  Fork for mbed/C++.
 @date      2017-03-29  T.Kawamura  Add template.
 
 @see 
 Copyright (C) 2016-2017 T.Kawamura.
 Released under the MIT license.
 http://opensource.org/licenses/mit-license.php
 
*/
 
#ifndef FIFO_H
#define FIFO_H
 
/**
     Disable all interuupts and save the status of interrupts.
     This macro is usable at ONLY Cortex-M Series. 
*/
#define DISABLE_INTERRUPTS uint32_t primask = __get_PRIMASK();  __disable_irq()
/**
     Enable all interuupts when the status of interrupts is ENABLED.
     This macro is usable at ONLY Cortex-M Series. 
*/
#define RESTORE_INTERUUPTS if( !(primask & 1) ) __enable_irq()
 
/**
    @class  FIFO
  @brief    Variable Length Software Ring Buffer
*/
template <typename T>
class FIFO{
 private:
    T *buffer;  // Buffer
  uint32_t size;    // Buffer Size
  volatile uint32_t getindex;   // Index for Getting Data
  volatile uint32_t putindex;   // Index for Putting Data
  volatile uint32_t count;  // Size of Data
    
 public:
    /**
        @brief  Create a new FIFO.
        @param  size    Buffer Size.
    */
    FIFO(uint32_t bufsize);
 
    /**
        @brief  Destroy the FIFO.
        @param  No Parameters.
    */
    virtual ~FIFO(void);
 
    /**
  @brief    Clear the buffer.
  @param    No Parameters.
    */
    virtual void clear(void);
 
    /**
  @brief    Get buffer size.
  @param    No Parameters.
  @return   Buffer size
    */
    virtual uint32_t getsize(void);
 
    /**
  @brief    Get byte from the buffer.
  @param    No Parameters.
  @retval   All Data
  @retval   0   Error.
    */
    virtual T get(void);
    
    /**
  @brief    Get byte from the buffer.
  @param    pop value refence
  @retval   true Success
  @retval   false Failure
    */
    virtual bool pop(T &pop);

    /**
      @brief    Peek byte from the buffer
        Peek 1byte from the buffer.(Do not touch index)
      @param    No Parameters.
      @retval   All Data
      @retval   0   Error.
    */  
    virtual T peek(void);
    
    /**
  @brief    Put byte to the buffer
  @param    No Parameters.
  @param    putdata     The Data for Putting.
  @return   Nothing.
    */
    virtual void put(const T putdata);
 
    /**
  @brief    Get Size of Retained Data.
  @param    No Parameters.
  @return   Data size.
    */
    virtual uint32_t available(void);
 
    /**
     *  @brief  Overloaded operator for putting data to the FIFO.
     *  @param  Data to put
     */
    FIFO &operator= (T data){
        put(data);
        return *this;
    }
 
    /**
     *  @brief  Overloaded operator for getting data from the FIFO.
     *  @param  No Parameters.
     *  @return Oldest Data
     */
    operator int(void){
        return get();
    }
    
};
 
#endif