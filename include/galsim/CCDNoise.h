// -*- c++ -*-
/*
 * Copyright 2012, 2013 The GalSim developers:
 * https://github.com/GalSim-developers
 *
 * This file is part of GalSim: The modular galaxy image simulation toolkit.
 *
 * GalSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GalSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GalSim.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef IMAGENOISE_H
#define IMAGENOISE_H

/** 
 * @file CCDNoise.h @brief Add noise to image using standard CCD model.
 *
 */

#include <cmath>
#include "Std.h"
#include "Random.h"
#include "Image.h"

namespace galsim {
    /** 
     * @brief Class implementing basic CCD noise model.  
     *
     * CcdNoise instance is given a "read noise" and gain level.  With these parameters set,
     * it can operate on an Image by adding noise to each pixel.  The noise has two
     * components: first, a Poisson deviate with variance equal to ( max(pixel value, 0) / gain);
     * second, Gaussian noise with RMS value of (readNoise / gain).  The class must
     * be given a reference to a UniformDeviate when constructed, which will be the source
     * of random values for the noise implementation.
     * readNoise=0 will shut off the Gaussian noise.
     * gain<=0 will shut off the Poisson noise, and Gaussian value will just have value 
     * RMS=readNoise.
     */
    class CCDNoise : public BaseDeviate
    {
    public:
        /**
         * @brief Construct a new noise model, using time of day as seed.
         *
         * @param[in] gain      Electrons per ADU in the input Images, used for Poisson noise.
         * @param[in] readNoise RMS of Gaussian noise, in electrons (if gain>0.) or ADU (gain<=0.)
         */
        CCDNoise(double gain=1., double readNoise=0.):
            BaseDeviate(),
            _gain(gain),
            _readNoise(readNoise),
            _gd(*this,0., 1.),
            _pd(*this) 
        {
            if (_readNoise > 0.) _gd.setSigma( _readNoise / (_gain > 0. ? _gain : 1.));
        }
 
        /**
         * @brief Construct a new noise model, using the provided value as seed.
         *
         * @param[in] lseed     Seed to use
         * @param[in] gain      Electrons per ADU in the input Images, used for Poisson noise.
         * @param[in] readNoise RMS of Gaussian noise, in electrons (if gain>0.) or ADU (gain<=0.)
         */
        CCDNoise(long lseed, double gain=1., double readNoise=0.):
            BaseDeviate(lseed),
            _gain(gain),
            _readNoise(readNoise),
            _gd(*this, 0., 1.),
            _pd(*this) 
        {
            if (_readNoise > 0.) _gd.setSigma( _readNoise / (_gain > 0. ? _gain : 1.));
        }
 
        /**
         * @brief Construct a new noise model, sharing the random number generator with dev.
         *
         * @param[in] dev       Other deviate with which to share the RNG
         * @param[in] gain      Electrons per ADU in the input Images, used for Poisson noise.
         * @param[in] readNoise RMS of Gaussian noise, in electrons (if gain>0.) or ADU (gain<=0.)
         */
        CCDNoise(const BaseDeviate& dev, double gain=1., double readNoise=0.) :
            BaseDeviate(dev),
            _gain(gain),
            _readNoise(readNoise),
            _gd(dev, 0., 1.),
            _pd(dev) 
        {
            if (_readNoise > 0.) _gd.setSigma( _readNoise / (_gain > 0. ? _gain : 1.));
        }

        /**
         * @brief Construct a copy that shares the RNG with rhs.
         *
         * Note: the default constructed op= function will do the same thing.
         */
        CCDNoise(const CCDNoise& rhs) : 
            BaseDeviate(rhs), _gain(rhs._gain), _readNoise(rhs._readNoise),
            _gd(rhs._gd), _pd(rhs._pd) {}
 

        /**
         * @brief Report current gain value
         *
         * @return Gain value (e/ADU)
         */
        double getGain() const { return _gain; }

        /**
         * @brief Report current read noise
         *
         * @return Read noise value (e, if gain>0, else in ADU)
         */
        double getReadNoise() const { return _readNoise; }

        /**
         * @brief Set gain value
         *
         * @param[in] gain Gain value (e/ADU)
         */
        void setGain(double gain) 
        {
            _gain = gain;
            if (_readNoise > 0.) _gd.setSigma( _readNoise / (_gain > 0. ? _gain : 1.));
        }
        /**
         * @brief Set read noise
         *
         * @param[in] readNoise Read noise value (e, if gain>0, else in ADU)
         */
        void setReadNoise(double readNoise) 
        {
            _readNoise = readNoise;
            if (_readNoise > 0.) _gd.setSigma( _readNoise / (_gain > 0. ? _gain : 1.));
        }

        /**
         * @brief Add noise to an Image.
         *
         * Poisson and/or Gaussian noise are added to each pixel of the image according
         * to standard CCD model.
         * @param[in,out] data The Image to be noise-ified.
         */
        template <typename T>
        void applyTo(ImageView<T> data) {
            // Above this many e's, assume Poisson distribution =Gaussian 
            static const double MAX_POISSON=1.e5;
            // Typedef for image row iterable
            typedef typename ImageView<T>::iterator ImIter;

            // Add the Poisson noise first:
            if (_gain > 0.) {
                double sigma = _gd.getSigma();  // Save this
                
                for (int y = data.getYMin(); y <= data.getYMax(); y++) {  // iterate over y
                    ImIter ee = data.rowEnd(y);
                    for (ImIter it = data.rowBegin(y); it != ee; ++it) {
                        double electrons=*it * _gain;
                        if (electrons <= 0.) continue;
                        if (electrons < MAX_POISSON) {
                            _pd.setMean(electrons);
                            *it = T(_pd() / _gain);
                        } else {
                            // ??? This might be even slower than large-N Poisson...
                            _gd.setSigma(sqrt(electrons)/_gain);
                            *it = T(*it + _gd());
                        }
                    } 
                }
                _gd.setSigma(sigma);    // restore GaussianDeviate's level
            }

            // Next add the Gaussian noise:
            if (_readNoise > 0.) {
                for (int y = data.getYMin(); y <= data.getYMax(); y++) {  // iterate over y
                    ImIter ee = data.rowEnd(y);
                    for (ImIter it = data.rowBegin(y); it != ee; ++it) { *it = T(*it + _gd()); }
                } 
            }
        }

        /**
         * @brief Add noise to an Image and also report variance of each pixel.
         *
         * Adds noise as in applyTo(Image) signature, but second Image is filled with
         * variance of added noise.  
         * @param[in,out] data The Image to be noise-ified.
         * @param[in,out] variance Image to fill with variance of applied noise.  Will be resized
         * if it does not match dimensions of data.
         */
        template <class T>
        void applyToVar(ImageView<T> data, ImageView<T> variance) {
            // Typedef for image row iterable
            typedef typename ImageView<T>::iterator ImIter;
            // Resize the variance image to match data image
            variance.resize(data.getBounds());
            // Fill with the (constant) Gaussian contribution to variance
            double gaussVariance = (_readNoise > 0. ? std::pow(_gd.getSigma(),2.) : 0.);
            variance.fill(gaussVariance);
            // Add the Poisson variance:
            if (_gain > 0.) {
                for (int y = data.getYMin(); y <= data.getYMax(); y++) {  // iterate over y
                    ImIter ee = data.rowEnd(y);
                    ImIter it2 = variance.rowBegin(y);
                    for (ImIter it = data.rowBegin(y); it != ee; ++it, ++it2) {
                        if (*it > 0.) *it2 += *it / _gain;
                    }
                } 
            }
            // then call noise method to instantiate noise
            (*this).applyTo(data);
        }

    private: 
        double _gain;    // flux corresponding to one photon
        double _readNoise; // std. dev. of uniform Gaussian noise (when divided by _gain).

        GaussianDeviate _gd;
        PoissonDeviate _pd;
    };

};  // namespace galsim

#endif