// -*- c++ -*-

//#define DEBUGLOGGING

#include <complex>
#include "Image.h"
#include "SBInterpolatedImageImpl.h"
#include "SBInterpolatedImage.h"
#include "CorrelatedNoise.h"

#ifdef DEBUGLOGGING
#include <fstream>
std::ostream* dbgout = new std::ofstream("debug.out");
int verbose_level = 2;
/*
 * There are three levels of verbosity which can be helpful when debugging, which are written as
 * dbg, xdbg, xxdbg (all defined in Std.h).
 * It's Mike's way to have debug statements in the code that are really easy to turn on and off.
 *
 * If DEBUGLOGGING is #defined, then these write out to *dbgout, according to the value of 
 * verbose_level.
 * dbg requires verbose_level >= 1
 * xdbg requires verbose_level >= 2
 * xxdbg requires verbose_level >= 3
 * If DEBUGLOGGING is not defined, the all three becomes just `if (false) std::cerr`,
 * so the compiler parses the statement fine, but trivially optimizes the code away, so there is no
 * efficiency hit from leaving them in the code.
 */
#endif

namespace galsim {

    template <typename T>
    SBNoiseCF::SBNoiseCF(
        const BaseImage<T>& image,
        boost::shared_ptr<Interpolant2d> xInterp, boost::shared_ptr<Interpolant2d> kInterp,
        double dx, double pad_factor) :
        SBInterpolatedImage(new SBNoiseCFImpl(image,xInterp,kInterp,dx,pad_factor)) {}

    SBNoiseCF::SBNoiseCF(const SBNoiseCF& rhs) : SBInterpolatedImage(rhs) {}
  
    SBNoiseCF::~SBNoiseCF() {}

    template <typename T>
    SBNoiseCF::SBNoiseCFImpl::SBNoiseCFImpl(
        const BaseImage<T>& image, 
        boost::shared_ptr<Interpolant2d> xInterp, boost::shared_ptr<Interpolant2d> kInterp,
        double dx, double pad_factor) : 
        SBInterpolatedImageImpl(image, xInterp, kInterp, dx, pad_factor) {}

    //
    template <typename T>
    Image<T> SBNoiseCF::getCovarianceMatrix(ImageView<T> image) const
    {
        int imin = image.getXMin();
        int jmin = image.getYMin();
        int imax = image.getXMax();
        int jmax = image.getYMax();
        int idim = 1 + imax - imin;
        int jdim = 1 + jmax - jmin;
        int covdim = idim * jdim;
        double dx = image.getScale();

        int k, ell; // k and l are indices that refer to image pixel separation vectors in the 
	            // correlation func.
        double x_k, y_ell; // physical vector separations in the correlation func, dx * k etc.
        Image<T> cov = Image<T>(covdim, covdim, T(0));
        for (int i=0; i<covdim; i++){

	  for (int j=i; j<covdim; j++){

            k = (j / jdim) - (i / idim);  // using integer division rules here
            ell = (j % jdim) - (i % idim);
            x_k = double(k) * dx;
            y_ell = double(ell) * dx;
            Position<T> p = Position<T>(x_k, y_ell);
	    cov.setValue(i, j, _pimpl->xValue(p));

          }

        }
	return cov;
    }

    // Here we redefine the xValue and kValue (as compared to the SBProfile versions) to enforce
    // two-fold rotational symmetry.

    double SBNoiseCF::SBNoiseCFImpl::xValue(const Position<double>& p) const 
    {
        if ( p.y <= 0. ) {
            return _xtab->interpolate(p.x, p.y, *_xInterp);
        } else {
            return _xtab->interpolate(-p.x, -p.y, *_xInterp);
        }
    }

    std::complex<double> SBNoiseCF::SBNoiseCFImpl::kValue(
        const Position<double> &p) const
    {
        const double TWOPI = 2.*M_PI;

        // Don't bother if the desired k value is cut off by the x interpolant:

        double ux = p.x*_multi.getScale()/TWOPI;
        if (std::abs(ux) > _xInterp->urange()) return std::complex<double>(0.,0.);
        double uy = p.y*_multi.getScale()/TWOPI;
        if (std::abs(uy) > _xInterp->urange()) return std::complex<double>(0.,0.);
        double xKernelTransform = _xInterp->uval(ux, uy);

        checkK();  // this, along with a bunch of other stuff, comes from the SBInterpolatedImage

        if ( p.y <= 0. ) {
            return xKernelTransform * _ktab->interpolate(p.x, p.y, *_kInterp);
        } else {
            return xKernelTransform * _ktab->interpolate(-p.x, -p.y, *_kInterp);
        }
    }

    // instantiate template functions for expected image types
    template SBNoiseCF::SBNoiseCF(
        const BaseImage<float>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCF(
        const BaseImage<double>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCF(
        const BaseImage<int>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCF(
        const BaseImage<short>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);

    template SBNoiseCF::SBNoiseCFImpl::SBNoiseCFImpl(
        const BaseImage<float>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCFImpl::SBNoiseCFImpl(
        const BaseImage<double>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCFImpl::SBNoiseCFImpl(
        const BaseImage<int>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);
    template SBNoiseCF::SBNoiseCFImpl::SBNoiseCFImpl(
        const BaseImage<short>& image, boost::shared_ptr<Interpolant2d> xInterp,
        boost::shared_ptr<Interpolant2d> kInterp, double dx, double pad_factor);

}
