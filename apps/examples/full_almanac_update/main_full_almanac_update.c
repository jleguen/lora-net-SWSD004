/*!
 * @file      main_full_almanac_update.c
 *
 * @brief     LR11xx full almanac update example
 *
 * @copyright
 * The Clear BSD License
 * Copyright Semtech Corporation 2022. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <time.h>

#include "smtc_board.h"
#include "smtc_hal.h"
#include "apps_modem_common.h"
#include "apps_modem_event.h"
#include "smtc_board_ralf.h"
#include "smtc_modem_utilities.h"

#include "lr11xx_gnss_types.h"

#include "almanac.h" /* Almanac image file, generated by the get_full_almanac.py python script provided */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#define xstr( a ) str( a )
#define str( a ) #a

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define OFFSET_BETWEEN_GPS_EPOCH_AND_UNIX_EPOCH 315964800

#define TIME_BUFFER_SIZE 80

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*!
 * @brief write full almanac image to LR11xx
 */
static bool almanac_update( const void* ral_context );

/*!
 * @brief Reset event callback
 *
 * @param [in] reset_count reset counter from the modem
 */
static void on_modem_reset( uint16_t reset_count );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/**
 * @brief Main application entry point.
 */
int main( void )
{
    static apps_modem_event_callback_t smtc_event_callback = {
        .adr_mobile_to_static  = NULL,
        .alarm                 = NULL,
        .almanac_update        = NULL,
        .down_data             = NULL,
        .join_fail             = NULL,
        .joined                = NULL,
        .link_status           = NULL,
        .mute                  = NULL,
        .new_link_adr          = NULL,
        .reset                 = on_modem_reset,
        .set_conf              = NULL,
        .stream_done           = NULL,
        .time_updated_alc_sync = NULL,
        .tx_done               = NULL,
        .upload_done           = NULL,
        .user_radio_access     = NULL,
        .middleware_1          = NULL,
    };

    ralf_t* modem_radio = smtc_board_initialise_and_get_ralf( );

    hal_mcu_disable_irq( );

    hal_mcu_init( );
    smtc_board_init_periph( );

    /* Init the Lora Basics Modem event callbacks */
    apps_modem_event_init( &smtc_event_callback );

    smtc_modem_init( modem_radio, &apps_modem_event_process );

    hal_mcu_enable_irq( );

    HAL_DBG_TRACE_INFO( "===== LoRa Basics Modem full almanac update example =====\n\n" );
    apps_modem_common_display_lbm_version( );

    /* Convert raw almanac date to epoch time */
    uint16_t almanac_date_raw = ( uint16_t )( ( full_almanac[2] << 8 ) | full_almanac[1] );
    time_t   almanac_date = ( OFFSET_BETWEEN_GPS_EPOCH_AND_UNIX_EPOCH + 24 * 3600 * ( 2048 * 7 + almanac_date_raw ) );

    /* Convert epoch time to human readbale format */
    char             buf[TIME_BUFFER_SIZE];
    const struct tm* time = localtime( &almanac_date );
    strftime( buf, TIME_BUFFER_SIZE, "%a %Y-%m-%d %H:%M:%S %Z", time );
    HAL_DBG_TRACE_PRINTF( "Source almanac date: %s\n\n", buf );

    /* Update full almanac */
    almanac_update( modem_radio->ral.context );

    while( 1 )
    {
        /* Execute modem runtime, this function must be called again in sleep_time_ms milliseconds or sooner. */
        uint32_t sleep_time_ms = smtc_modem_run_engine( );

        hal_mcu_set_sleep_for_ms( sleep_time_ms );
    }
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/*!
 * @brief LoRa Basics Modem event callbacks called by smtc_event_process function
 */

static void on_modem_reset( uint16_t reset_count )
{
    /* Notify user with leds */
    smtc_board_led_pulse( smtc_board_get_led_tx_mask( ), true, 250 );
}

static bool get_almanac_crc( const void* ral_context, uint32_t* almanac_crc )
{
    lr11xx_status_t                         err;
    lr11xx_gnss_context_status_bytestream_t context_status_bytestream;
    lr11xx_gnss_context_status_t            context_status;

    err = lr11xx_gnss_get_context_status( ral_context, context_status_bytestream );
    if( err != LR11XX_STATUS_OK )
    {
        HAL_DBG_TRACE_ERROR( "Failed to get gnss context status\n" );
        return false;
    }

    err = lr11xx_gnss_parse_context_status_buffer( context_status_bytestream, &context_status );
    if( err != LR11XX_STATUS_OK )
    {
        HAL_DBG_TRACE_ERROR( "Failed to parse gnss context status to get almanac status\n" );
        return false;
    }

    *almanac_crc = context_status.global_almanac_crc;

    return true;
}

static bool almanac_update( const void* ral_context )
{
    uint32_t global_almanac_crc, local_almanac_crc;
    local_almanac_crc =
        ( full_almanac[6] << 24 ) + ( full_almanac[5] << 16 ) + ( full_almanac[4] << 8 ) + ( full_almanac[3] );

    if( get_almanac_crc( ral_context, &global_almanac_crc ) == false )
    {
        HAL_DBG_TRACE_ERROR( "Failed to get almanac CRC before update\n" );
        return false;
    }
    if( global_almanac_crc != local_almanac_crc )
    {
        HAL_DBG_TRACE_INFO( "Local almanac doesn't match LR11XX almanac -> start update\n" );

        /* Load almanac in flash */
        uint16_t almanac_idx = 0;
        while( almanac_idx < sizeof( full_almanac ) )
        {
            if( lr11xx_gnss_almanac_update( ral_context, full_almanac + almanac_idx, 1 ) != LR11XX_STATUS_OK )
            {
                HAL_DBG_TRACE_ERROR( "Failed to update almanac\n" );
                return false;
            }
            almanac_idx += LR11XX_GNSS_SINGLE_ALMANAC_WRITE_SIZE;
        }

        /* Check CRC again to confirm proper update */
        if( get_almanac_crc( ral_context, &global_almanac_crc ) == false )
        {
            HAL_DBG_TRACE_ERROR( "Failed to get almanac CRC after update\n" );
            return false;
        }
        if( global_almanac_crc != local_almanac_crc )
        {
            HAL_DBG_TRACE_ERROR( "Local almanac doesn't match LR11XX almanac -> update failed\n" );
            return false;
        }
        else
        {
            HAL_DBG_TRACE_INFO( "Almanac update succeeded\n" );
        }
    }
    else
    {
        HAL_DBG_TRACE_INFO( "Local almanac matches LR11XX almanac -> no update\n" );
    }

    return true;
}

/* --- EOF ------------------------------------------------------------------ */