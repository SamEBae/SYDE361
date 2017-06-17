/**
 *  @file ToneDetector.cpp
 *
 *  @date 16.04.2014
 *  @author Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *  License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *  Sources: https://github.com/ArminJo/arduino-tone-detector
 *
 *  @version 1.0
 *
 *  @brief analyzes a microphone signal and outputs the detected frequency.
 *
 *  readSignal() is the ADC read routine, which reads 1 + 1024/512 samples and computes frequency of signal
 *
 *  doPlausi() checks if the signal is not noisy and valid. It uses the following plausibility rules:
 *      1. A trigger must be detected in first and last 1/8 of samples
 *      2. Only 1/8 of the samples are allowed to be greater than 1.5 or less than 0.75 of the average period
 *
 *  computeDirectAndFilteredMatch() wait for n matches within a given frequency range (FrequencyMatchLow - FrequencyMatchHigh)
 *  and also low pass filters the result for smooth transitions between the 3 match states (lower, match, greater)
 *
 *
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
//#define DEBUG
//#define TRACE
#else
#include  "UART_TINY_1MHz_38400.h"
#endif

#include <Arduino.h>
#include <ToneDetector.h>

#define maximumAllowableCountOf(aPeriodCountTotal) (aPeriodCountTotal / 8)

/*
 * values for low pass filtering the match result
 */
#define FILTER_VALUE_MAX        0x200
#define FILTER_VALUE_MIN        0
#define FILTER_VALUE_MIDDLE     ((FILTER_VALUE_MAX + FILTER_VALUE_MIN)/2)
#define FILTER_VALUE_THRESHOLD  (FILTER_VALUE_MIDDLE/2)
#define FILTER_VALUE_MATCH      FILTER_VALUE_MIDDLE
#define FILTER_VALUE_MATCH_HIGHER_THRESHOLD     (FILTER_VALUE_MAX - FILTER_VALUE_THRESHOLD)
#define FILTER_VALUE_MATCH_LOWER_THRESHOLD      (FILTER_VALUE_MIN + FILTER_VALUE_THRESHOLD)

ToneDetectorControlStruct ToneDetectorControl;

// Union to speed up the combination of low and high bytes to a word
// it is not optimal since the compiler still generates 2 unnecessary moves
// but using  -- value = (high << 8) | low -- gives 5 unnecessary instructions
union Myword {
    struct {
        uint8_t LowByte;
        uint8_t HighByte;
    } byte;
    uint16_t Word;
    uint8_t * BytePointer;
};

/*
 * aADCChannel can be 0 to 7 or A0 to A7
 * aADCReference can be DEFAULT (VCC) or INTERNAL (1.1V)
 * aADCPrescalerValue can be one of PRESCALE4, PRESCALE8, PRESCALE32, PRESCALE64 or PRESCALE128
 * aFrequencyOfOneSampleTimes100 depends on value of aADCPrescalerValue
 * Formula is
 */
void setToneDetectorReadingValues(uint8_t aADCChannel, uint8_t aADCReference, uint8_t aADCPrescalerValue,
        uint16_t aRawVoltageMinDelta) {

    // Setup ADC, setMUX + Reference
    if (aADCChannel >= 14) {
        aADCChannel -= 14; // allow for channel or pin numbers
    }
    aADCReference = aADCReference << 6;
    ADMUX = aADCReference | aADCChannel;

    setToneDetectorReadingPrescaleValue(aADCPrescalerValue);
    ToneDetectorControl.RawVoltageMinDelta = aRawVoltageMinDelta;
}

void setToneDetectorReadingPrescaleValue(uint8_t aADCPrescalerValue) {
    ToneDetectorControl.ADCPrescalerValue = aADCPrescalerValue;
    //Formula is F_CPU / (PrescaleFactor * 13)
    ToneDetectorControl.PeriodOfOneSampleMicros = ((1 << aADCPrescalerValue) * 13) / (F_CPU / 1000000L);
    uint32_t tFrequencyOfOneSample = 1000000L / ToneDetectorControl.PeriodOfOneSampleMicros;
    ToneDetectorControl.FrequencyOfOneSample = tFrequencyOfOneSample;
#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
    Serial.print("SamplePeriod=");
    Serial.print(ToneDetectorControl.PeriodOfOneSampleMicros);
    Serial.println("us");
#else
    writeString(" SamplePeriod=");
    writeInt(ToneDetectorControl.PeriodOfOneSampleMicros);
    writeString("us");
#endif
}

void setToneDetectorMatchValues(uint16_t aFrequencyMin, uint16_t aFrequencyMax) {
    ToneDetectorControl.FrequencyMatchLow = aFrequencyMin;
    ToneDetectorControl.FrequencyMatchHigh = aFrequencyMax;
}

void setToneDetectorDropoutValues(uint8_t aMinMatchNODropoutCount, uint8_t aMaxMatchDropoutCount) {
    ToneDetectorControl.MinMatchNODropoutCount = aMinMatchNODropoutCount;
    ToneDetectorControl.MaxMatchDropoutCount = aMaxMatchDropoutCount;
}

void setToneDetectorControlDefaults(void) {
    setToneDetectorReadingValues(ADC_CHANNEL_DEFAULT, INTERNAL, PRESCALE_VALUE_DEFAULT, RAW_VOLTAGE_MIN_DELTA_DEFAULT);
    setToneDetectorMatchValues(FREQUENCY_MIN_DEFAULT, FREQUENCY_MAX_DEFAULT);
    setToneDetectorDropoutValues(MIN_NO_DROPOUT_COUNT_BEFORE_ANY_MATCH_DEFAULT, MAX_DROPOUT_COUNT_BEFORE_NO_FILTERED_MATCH_DEFAULT);
}

/*
 * ADC read routine reads 1 + 1024 (512) samples and computes:
 * - ToneDetectorControl.FrequencyActual - Frequency of signal
 * or error value SIGNAL_STRENGTH_LOW if signal is too weak
 *
 * Sets the next trigger levels for hysteresis of 1/8 peak to peak value:
 * - ToneDetectorControl.TriggerLevelLower - for next reading
 * - ToneDetectorControl.TriggerLevelUpper - for next reading
 *
 * Sets values for plausibility check:
 * - ToneDetectorControl.PeriodLength[]
 * - ToneDetectorControl.PeriodCountActual
 * - ToneDetectorControl.TriggerFirstPosition
 * - ToneDetectorControl.TriggerLastPosition
 * Triggering value for next reading, trigger hysteresis is 1/8 peak to peak value
 */
uint16_t readSignal() {
    /**********************************
     * wait for triggering condition
     **********************************/
    Myword tUValue;
    bool tTriggerSearchStart = true;
    bool tSignalTriggerFound = false;
    uint16_t i;
    uint16_t tValueMax;
    uint16_t tValueMin;
    uint16_t tPeriodCountPosition = 0;
    uint8_t tPeriodCountActual;

    /*
     * Read 513 samples
     * First sample is only used to initialize min and max values
     */
//  ADCSRB = 0; // free running mode  - is default
    ADCSRA = ((1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIF) | ToneDetectorControl.ADCPrescalerValue);

// wait for free running conversion to finish
    while (bit_is_clear(ADCSRA, ADIF)) {
        ;
    }
    tUValue.byte.LowByte = ADCL;
    tUValue.byte.HighByte = ADCH;

// without "| (1 << ADSC)" it does not work - undocumented feature???
    ADCSRA |= (1 << ADIF) | (1 << ADSC);

    // Initialize max and min with first value
    tValueMax = tUValue.Word;
    tValueMin = tUValue.Word;

    //digitalToggleFast(LED_DEBUG);
    tPeriodCountActual = 0;

    for (i = 0; i < NUMBER_OF_SAMPLES; i++) {
        // wait for free running conversion to finish
        while (bit_is_clear(ADCSRA, ADIF)) {
            ;
        }
        // Get value
        tUValue.byte.LowByte = ADCL;
        tUValue.byte.HighByte = ADCH;
        // without "| (1 << ADSC)" it does not work - undocumented feature???
        ADCSRA |= (1 << ADIF) | (1 << ADSC); // clear bit to recognize next conversion has finished

        /*
         * Detect trigger
         */
        if (tTriggerSearchStart) {
            // rising slope - wait for value below 1. threshold
            if (tUValue.Word < ToneDetectorControl.TriggerLevelLower) {
                tTriggerSearchStart = false;
            }
        } else {
            // rising slope - wait for value to rise above 2. threshold
            if (tUValue.Word > ToneDetectorControl.TriggerLevelUpper) {
                //Trigger found but skip first (incomplete period)
                if (tSignalTriggerFound) {
                    ToneDetectorControl.PeriodLength[tPeriodCountActual] = i - tPeriodCountPosition;
                    if (tPeriodCountActual < (sizeof(ToneDetectorControl.PeriodLength) - 1)) {
                        tPeriodCountActual++;
                    }
                    tPeriodCountPosition = i;
                } else {
                    ToneDetectorControl.TriggerFirstPosition = i;
                    tPeriodCountPosition = i;
                }
                tSignalTriggerFound = true;
                tTriggerSearchStart = true;
            }
        }

        // get max and min for automatic triggering - needs 0,4 microseconds
        if (tUValue.Word > tValueMax) {
            tValueMax = tUValue.Word;
        } else if (tUValue.Word < tValueMin) {
            tValueMin = tUValue.Word;
        }

    }
    ADCSRA &= ~(1 << ADATE); // Disable auto-triggering

    ToneDetectorControl.TriggerLastPosition = tPeriodCountPosition;
    ToneDetectorControl.PeriodCountActual = tPeriodCountActual;
    /*
     * check for signal strength
     */
    uint16_t tDelta = tValueMax - tValueMin;
    if (tDelta < ToneDetectorControl.RawVoltageMinDelta) {
        ToneDetectorControl.FrequencyActual = SIGNAL_STRENGTH_LOW;
        return SIGNAL_STRENGTH_LOW; // don't compute new trigger value because signal is too low!!!!
    }

// middle between min and max
    uint16_t tTriggerValue = tValueMin + (tDelta / 2);
    // set hysteresis
    uint8_t TriggerHysteresis = tDelta / 8;
    ToneDetectorControl.TriggerLevelUpper = tTriggerValue;
    ToneDetectorControl.TriggerLevelLower = tTriggerValue - TriggerHysteresis;

    if (tPeriodCountActual == 0) {
        return SIGNAL_NO_TRIGGER; // Frequency too low
    }

    /*
     * Must use long intermediate value to avoid 16Bit overflow
     */
    ToneDetectorControl.FrequencyActual = ((long) tPeriodCountActual * ToneDetectorControl.FrequencyOfOneSample)
            / (ToneDetectorControl.TriggerLastPosition - ToneDetectorControl.TriggerFirstPosition);

#ifdef DEBUG
    Serial.print("Delta U=");
    Serial.print(tDelta);
    Serial.print(" TriggerValue=");
    Serial.print(tTriggerValue);
    Serial.print(" PeriodCount=");
    Serial.print(ToneDetectorControl.PeriodCountActual);
    Serial.print(" Samples=");
    Serial.print(ToneDetectorControl.TriggerLastPosition - ToneDetectorControl.TriggerFirstPosition);
    Serial.print(" Freq=");
    Serial.println(ToneDetectorControl.FrequencyActual);
#endif

    return ToneDetectorControl.FrequencyActual;
}

/** Overwrite ToneDetectorControl.FrequencyActual with these error values if plausibility check fails:
 *      SIGNAL_FIRST_PLAUSI_FAILED 1
 *      SIGNAL_DISTRIBUTION_PLAUSI_FAILED 2
 * Used plausibility rules are:
 * 1. A trigger must be detected in first and last 1/8 of samples
 * 2. Only 1/8 of the samples are allowed to be greater than 1.5 or less than 0.75 of the average period
 * @return the (changed) ToneDetectorControl.FrequencyActual
 */
uint16_t doPlausi() {
    if (ToneDetectorControl.FrequencyActual > SIGNAL_MAX_ERROR_CODE) {
        /*
         * plausibility check
         */
        if (ToneDetectorControl.TriggerFirstPosition >= LEADING_TRAILING_TRIGGER_MARGIN
                || ToneDetectorControl.TriggerLastPosition <= NUMBER_OF_SAMPLES - LEADING_TRAILING_TRIGGER_MARGIN) {
            ToneDetectorControl.FrequencyActual = SIGNAL_FIRST_LAST_PLAUSI_FAILED;

        } else {
            uint8_t tPeriodCountActual = ToneDetectorControl.PeriodCountActual;
            /*
             * check if not more than 1/8 of periods are out of range - less than 0.75 or more than 1.5
             */
            // average can be between 8 an 32
            uint8_t tAveragePeriod = (ToneDetectorControl.TriggerLastPosition - ToneDetectorControl.TriggerFirstPosition)
                    / tPeriodCountActual;
            uint8_t tPeriodMax = tAveragePeriod + (tAveragePeriod / 2);
            uint8_t tPeriodMin = tAveragePeriod - (tAveragePeriod / 4);
            uint8_t tErrorCount = 0;
#ifdef TRACE
            Serial.print(tAveragePeriod);
            Serial.print("  ");
#endif
            for (uint8_t i = 0; i < tPeriodCountActual; ++i) {
#ifdef TRACE
                Serial.print(ToneDetectorControl.PeriodLength[i]);
                Serial.print(",");
#endif
                if (ToneDetectorControl.PeriodLength[i] > tPeriodMax || ToneDetectorControl.PeriodLength[i] < tPeriodMin) {
                    tErrorCount++;
                }
            }
            if (tErrorCount > maximumAllowableCountOf(tPeriodCountActual)) {
                ToneDetectorControl.FrequencyActual = SIGNAL_DISTRIBUTION_PLAUSI_FAILED;
            }
#ifdef TRACE
            Serial.print(tErrorCount);
            Serial.print("  #=");
            Serial.print(tPeriodCountActual);
            Serial.print("  F=");
            Serial.print(ToneDetectorControl.FrequencyActual);
            Serial.println("Hz");

#endif
        }
    }
    return ToneDetectorControl.FrequencyActual;
}

/**
 * simple low-pass filter
 * history of 15 values
 */
uint16_t filter(uint16_t aFilteredValue, uint16_t aValue) {
    uint16_t tValue = (aFilteredValue << 4) - aFilteredValue; // aFilteredValue * 15
    tValue += aValue + (1 << 3); // to avoid rounding errors
    return (tValue >> 4); // tValue/16
}

/**
 * handles dropouts / no signal
 * dropout count is between 0 and MaxMatchDropoutCount and on latter the match will be reseted.
 * determine direct match state - ToneDetectorControl.ToneMatch
 * computes low-pass filtered match state - ToneDetectorControl.ToneMatchFiltered
 */
void computeDirectAndFilteredMatch(uint16_t aFrequency) {

    if (aFrequency <= SIGNAL_MAX_ERROR_CODE) {
        /*
         *  dropout / no signal handling
         */
        ToneDetectorControl.ToneMatchDirect = TONE_MATCH_INVALID;
        ToneDetectorControl.MatchDropoutCount++;
        if (ToneDetectorControl.MatchDropoutCount > ToneDetectorControl.MaxMatchDropoutCount) {
            ToneDetectorControl.ToneMatchFiltered = TONE_MATCH_INVALID;
            // clip value
            if (ToneDetectorControl.MatchDropoutCount
                    >= ToneDetectorControl.MaxMatchDropoutCount + ToneDetectorControl.MinMatchNODropoutCount) {
                ToneDetectorControl.MatchDropoutCount = ToneDetectorControl.MaxMatchDropoutCount
                        + ToneDetectorControl.MinMatchNODropoutCount;
            }
        }
    } else {
        /*
         * Determine direct match state and tNewFilterValue for low pass filter
         */
        uint16_t tNewFilterValue;
        if (aFrequency < ToneDetectorControl.FrequencyMatchLow) {
            // Tone too low
            ToneDetectorControl.ToneMatchDirect = TONE_MATCH_LOWER;
            tNewFilterValue = FILTER_VALUE_MIN;
        } else if (aFrequency > ToneDetectorControl.FrequencyMatchHigh) {
            // Tone too high
            ToneDetectorControl.ToneMatchDirect = TONE_MATCH_HIGHER;
            tNewFilterValue = FILTER_VALUE_MAX;
        } else {
            // Tone matches
            ToneDetectorControl.ToneMatchDirect = TONE_MATCH;
            tNewFilterValue = FILTER_VALUE_MATCH;
        }

        /*
         * Low pass filter for smooth transitions between the 3 match states (lower, match, greater)
         */
        if (ToneDetectorControl.MatchDropoutCount > ToneDetectorControl.MaxMatchDropoutCount) {
            // init filter at first time after no signal
            ToneDetectorControl.MatchLowPassFiltered = tNewFilterValue;
            ToneDetectorControl.FrequencyFiltered = aFrequency;
        } else {
            ToneDetectorControl.FrequencyFiltered = filter(ToneDetectorControl.FrequencyFiltered, aFrequency);
            ToneDetectorControl.MatchLowPassFiltered = filter(ToneDetectorControl.MatchLowPassFiltered, tNewFilterValue);
        }
        // decrement dropout count until 0
        // decrement MUST be placed here!
        if (ToneDetectorControl.MatchDropoutCount > 0) {
            ToneDetectorControl.MatchDropoutCount--;
        }

        /*
         * determine new low pass filtered match state
         * must be done after decrement of MatchDropoutCount since on last initialization also first evaluation must be done
         */
        if (ToneDetectorControl.MatchDropoutCount > ToneDetectorControl.MaxMatchDropoutCount) {
            ToneDetectorControl.ToneMatchFiltered = TONE_MATCH_INVALID;
        } else {
            if (ToneDetectorControl.MatchLowPassFiltered > FILTER_VALUE_MATCH_HIGHER_THRESHOLD) {
                ToneDetectorControl.ToneMatchFiltered = TONE_MATCH_HIGHER;
            } else if (ToneDetectorControl.MatchLowPassFiltered < FILTER_VALUE_MATCH_LOWER_THRESHOLD) {
                ToneDetectorControl.ToneMatchFiltered = TONE_MATCH_LOWER;
            } else {
                ToneDetectorControl.ToneMatchFiltered = TONE_MATCH;
            }
        }
    }
}
