// fill_style.cpp:  Graphical region filling styles, for Gnash.
// 
//   Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// Based on work of Thatcher Ulrich <tu@tulrich.com> 2003

#include "smart_ptr.h"
#include "fill_style.h"
#include "log.h"
#include "SWFStream.h"
#include "movie_definition.h"
#include "SWF.h"
#include "GnashException.h"
#include "GnashNumeric.h"
#include "Renderer.h"
#include "RunResources.h"
#include "GnashImage.h"

#include <cmath> // sqrt, floor
#include <iostream> // for output operator
#include <boost/variant.hpp>

namespace gnash {

// Forward declarations
namespace {
    rgba sampleGradient(const GradientFill& fill, boost::uint8_t ratio);
}

namespace {

#if 1
struct ConstMatrix : public boost::static_visitor<const SWFMatrix&>
{
    const SWFMatrix& operator()(const GradientFill& f) const {
        return f.matrix;
    }
    const SWFMatrix& operator()(const BitmapFill& f) const {
        return f.matrix;
    }
    const SWFMatrix& operator()(const SolidFill&) const {
        throw boost::bad_visit();
    }
};
#endif

/// Create a lerped version of two other fill_styles.
//
/// The two fill styles must have exactly the same types. Callers are 
/// responsible for ensuring this.
class SetLerp : public boost::static_visitor<>
{
public:
    SetLerp(const fill_style::Fill& a, const fill_style::Fill& b, double ratio)
        :
        _a(a),
        _b(b),
        _ratio(ratio)
    {
    }

    /// Create a lerped SolidFill.
    void operator()(SolidFill& f) const {
        const SolidFill& a = boost::get<SolidFill>(_a);
        const SolidFill& b = boost::get<SolidFill>(_b);
        f.color.set_lerp(a.color, b.color, _ratio);
    }

    /// Create a lerped GradientFill.
    void operator()(GradientFill& f) const {
        const GradientFill& a = boost::get<GradientFill>(_a);
        const GradientFill& b = boost::get<GradientFill>(_b);
        f.type = a.type;
        f.color.set_lerp(a.color, b.color, _ratio);
        
        // fill style gradients
        assert(f.gradients.size() == a.gradients.size());
        assert(f.gradients.size() == b.gradients.size());
        for (size_t i = 0, e = f.gradients.size(); i < e; ++i) {
            f.gradients[i].m_ratio = frnd(flerp(a.gradients[i].m_ratio,
                        b.gradients[i].m_ratio, _ratio));
            f.gradients[i].m_color.set_lerp(a.gradients[i].m_color,
                    b.gradients[i].m_color, _ratio);
        }
        f.gradientBitmap = 0;
        f.matrix.set_lerp(a.matrix, b.matrix, _ratio);
    }

    /// Create a lerped BitmapFill.
    void operator()(BitmapFill& f) const {
        const BitmapFill& a = boost::get<BitmapFill>(_a);
        const BitmapFill& b = boost::get<BitmapFill>(_b);
        f.bitmapInfo = a.bitmapInfo;
        f.matrix.set_lerp(a.matrix, b.matrix, _ratio);
    }


private:
    const fill_style::Fill& _a;
    const fill_style::Fill& _b;
    const double _ratio;

};

struct Matrix : public boost::static_visitor<SWFMatrix&>
{
    SWFMatrix& operator()(GradientFill& f) const {
        return f.matrix;
    }
    SWFMatrix& operator()(BitmapFill& f) const {
        return f.matrix;
    }
    SWFMatrix& operator()(SolidFill&) const {
        throw boost::bad_visit();
    }
};

struct ConstColor : public boost::static_visitor<const rgba&>
{
    const rgba& operator()(const GradientFill& f) const {
        return f.color;
    }
    const rgba& operator()(const SolidFill& f) const {
        return f.color;
    }
    const rgba& operator()(const BitmapFill&) const {
        throw boost::bad_visit();
    }

};

struct Color : public boost::static_visitor<rgba&>
{
    rgba& operator()(GradientFill& f) const {
        return f.color;
    }
    rgba& operator()(SolidFill& f) const {
        return f.color;
    }
    rgba& operator()(BitmapFill&) const {
        throw boost::bad_visit();
    }

};

}

rgba
fill_style::get_color() const
try {
    return boost::apply_visitor(ConstColor(), _fill);
}
catch (const boost::bad_visit&) {
    return rgba();
}

void
fill_style::set_color(rgba new_color)
try {
    boost::apply_visitor(Color(), _fill) = new_color;
}
catch (const boost::bad_visit&) {
}

void
gradient_record::read(SWFStream& in, SWF::TagType tag)
{
    in.ensureBytes(1);
    m_ratio = in.read_u8();
    m_color.read(in, tag);
}

fill_style::fill_style()
    :
    _fill(SolidFill(rgba(255, 255, 255, 255))),
    m_type(SWF::FILL_SOLID)
{
}

void
fill_style::read(SWFStream& in, SWF::TagType t, movie_definition& md,
        const RunResources& r, fill_style *pOther)
{
    const bool is_morph = (pOther != NULL);

    in.ensureBytes(1);
    m_type = in.read_u8();
    if (is_morph) {
        pOther->m_type = m_type;
    }
        
    IF_VERBOSE_PARSE(
        log_parse("  fill_style read type = 0x%X", +m_type);
    );

    switch (m_type) {

        case SWF::FILL_SOLID:
        {
            log_debug("Fill Solid!");
            rgba color;

            // 0x00: solid fill
            if (t == SWF::DEFINESHAPE3 || t == SWF::DEFINESHAPE4
                || t == SWF::DEFINESHAPE4_ || is_morph) {
                color.read_rgba(in);
                if (is_morph) {
                    rgba othercolor;
                    othercolor.read_rgba(in);
                    pOther->_fill = SolidFill(othercolor);
                }
            }
            else {
                // For DefineMorphShape tags we should use morph_fill_style 
                assert(t == SWF::DEFINESHAPE || t == SWF::DEFINESHAPE2);
                color.read_rgb(in);
            }

            IF_VERBOSE_PARSE(
                log_parse("  color: %s", color);
            );
            _fill = SolidFill(color);
            return;
        }

        case SWF::FILL_LINEAR_GRADIENT:
        case SWF::FILL_RADIAL_GRADIENT:
        case SWF::FILL_FOCAL_GRADIENT:
        {
            // 0x10: linear gradient fill
            // 0x12: radial gradient fill
            // 0x13: focal gradient fill

            GradientFill gf;

            SWFMatrix input_matrix;
            input_matrix.read(in);

            // shouldn't this be in initializer's list ?
            if (m_type == SWF::FILL_LINEAR_GRADIENT) {
                gf.type = GradientFill::LINEAR;
                gf.matrix.set_translation(128, 0);
                gf.matrix.set_scale(1.0/128, 1.0/128);
            }
            else {
                // FILL_RADIAL_GRADIENT or FILL_FOCAL_GRADIENT
                gf.matrix.set_translation(32, 32);
                gf.matrix.set_scale(1.0/512, 1.0/512);
                gf.type = m_type == SWF::FILL_FOCAL_GRADIENT ?
                    GradientFill::FOCAL : GradientFill::RADIAL;
            }

            SWFMatrix m = input_matrix;
            m.invert();

            if (is_morph) {
                pOther->_fill = GradientFill();
                boost::get<GradientFill>(pOther->_fill).matrix = gf.matrix;
            }

            gf.matrix.concatenate(m);
            
            if (is_morph) {
                input_matrix.read(in);
                m = input_matrix;
                m.invert();
                boost::get<GradientFill>(pOther->_fill).matrix.concatenate(m);
            }
            
            // GRADIENT
            in.ensureBytes(1);
            const boost::uint8_t grad_props = in.read_u8();
        
            if (t == SWF::DEFINESHAPE4 || t == SWF::DEFINESHAPE4_) {

                const boost::uint8_t spread_mode = grad_props >> 6;
                switch(spread_mode) {
                    case 0:
                        gf.spreadMode = SWF::GRADIENT_SPREAD_PAD;
                        break;
                    case 1:
                        gf.spreadMode = SWF::GRADIENT_SPREAD_REFLECT;
                        break;
                    case 2:
                        gf.spreadMode = SWF::GRADIENT_SPREAD_REPEAT;
                        break;
                    default: 
                        IF_VERBOSE_MALFORMED_SWF(
                            log_swferror("Illegal spread mode in gradient "
                                "definition.");
                        );
                }
        
                // TODO: handle in GradientFill.
                const boost::uint8_t interpolation = (grad_props >> 4) & 3;
                switch (interpolation) {
                    case 0: 
                        gf.interpolation = SWF::GRADIENT_INTERPOLATION_NORMAL;
                        break;
                    case 1:
                        gf.interpolation = SWF::GRADIENT_INTERPOLATION_LINEAR;
                        break;
                    default:
                        IF_VERBOSE_MALFORMED_SWF(
                            log_swferror("Illegal interpolation mode in "
                                "gradient definition.");
                        );
                }
            }
        
            const boost::uint8_t num_gradients = grad_props & 0xF;
            if (!num_gradients) {
                IF_VERBOSE_MALFORMED_SWF(
                    log_swferror(_("num gradients 0"));
                );
                return;
            }
        
            if (num_gradients > 8 + ((t == SWF::DEFINESHAPE4 ||
                t == SWF::DEFINESHAPE4_) ? 7 : 0)) {
               // see: http://sswf.sourceforge.net/SWFalexref.html#swf_gradient
                IF_VERBOSE_MALFORMED_SWF(
                    log_swferror(_("Unexpected num gradients (%d), "
                            "expected 1 to 8"), static_cast<int>(num_gradients));
                );
            }
        
            if (is_morph) {
                boost::get<GradientFill>(pOther->_fill).gradients.resize(
                        num_gradients);
            }
                    
            gf.gradients.resize(num_gradients);
            for (size_t i = 0; i < num_gradients; ++i) {
                gf.gradients[i].read(in, t);
                if (is_morph) {
                    boost::get<GradientFill>(pOther->_fill).gradients[i].read(
                            in, t);
                }
            }
        
            // A focal gradient also has a focal point.
            if (m_type == SWF::FILL_FOCAL_GRADIENT) {
               in.ensureBytes(2);
               gf.focalPoint = in.read_short_sfixed();
               if (gf.focalPoint < -1.0f) gf.focalPoint = -1.0f;
               else if (gf.focalPoint > 1.0f) gf.focalPoint = 1.0f;
            }
        
            if (is_morph) {
                boost::get<GradientFill>(pOther->_fill).focalPoint =
                    gf.focalPoint;
            }
        
            IF_VERBOSE_PARSE(
               log_parse("  gradients: num_gradients = %d", +num_gradients);
            );
        
            // @@ hack. What is it supposed to do?
            if (num_gradients > 0) {
                gf.color = gf.gradients[0].m_color;
                if (is_morph) {
                    boost::get<GradientFill>(pOther->_fill).color =
                        boost::get<GradientFill>(
                                pOther->_fill).gradients[0].m_color;
                }
            }
            
            // Do this before creating bitmap!
            _fill = gf;
        
            Renderer* renderer = r.renderer();
            if (renderer) {

                gf.gradientBitmap = gf.createBitmap(*renderer);
                if (is_morph) {
                    boost::get<GradientFill>(pOther->_fill).gradientBitmap =
                        pOther->need_gradient_bitmap(*renderer);
                }
            }
            return;
        }

        case SWF::FILL_TILED_BITMAP_HARD:
        case SWF::FILL_CLIPPED_BITMAP_HARD:
        case SWF::FILL_TILED_BITMAP:
        case SWF::FILL_CLIPPED_BITMAP:
        {
            BitmapFill bf;

            // 0x40: tiled bitmap fill
            // 0x41: clipped bitmap fill
            // 0x42: tiled bitmap fill with hard edges
            // 0x43: clipped bitmap fill with hard edges

            if (m_type == SWF::FILL_TILED_BITMAP_HARD ||
                 m_type == SWF::FILL_CLIPPED_BITMAP_HARD) {
                bf.smoothingPolicy = BitmapFill::SMOOTHING_OFF;
            }
            else if (md.get_version() >= 8) {
                bf.smoothingPolicy = BitmapFill::SMOOTHING_ON;
            }
            else {
                bf.smoothingPolicy = BitmapFill::SMOOTHING_UNSPECIFIED;
            }

            in.ensureBytes(2);
            int bitmap_char_id = in.read_u16();
            IF_VERBOSE_PARSE(
                log_parse("  bitmap_char = %d, smoothing_policy = %s",
                    bitmap_char_id, bf.smoothingPolicy);
            );

            // Look up the bitmap DisplayObject.
            bf.bitmapInfo = md.getBitmap(bitmap_char_id);
            IF_VERBOSE_MALFORMED_SWF(
                if (!bf.bitmapInfo) {
                    LOG_ONCE(
                        log_swferror(_("Bitmap fill specifies '%d' as associated"
                            " bitmap DisplayObject id,"
                            " but that DisplayObject is not found"
                            " in the Characters Dictionary."
                            " It seems common to find such "
                            " malformed SWF, so we'll only warn once "
                            "about this."), bitmap_char_id);
                    );
                }
            );

            SWFMatrix m;
            m.read(in);

            // For some reason, it looks like they store the inverse of the
            // TWIPS-to-texcoords SWFMatrix.
            bf.matrix = m.invert();

            if (is_morph) {
                pOther->_fill = BitmapFill();
                boost::get<BitmapFill>(pOther->_fill).bitmapInfo = bf.bitmapInfo;
                m.read(in);
                boost::get<BitmapFill>(pOther->_fill).matrix = m.invert();
            }

            IF_VERBOSE_PARSE(
               log_parse("SWFMatrix: %s", bf.matrix);
            );
            _fill = bf;
            return;
        }

        default:
        {
            std::stringstream ss;
            ss << "Unknown fill style type " << m_type;    
            // This is a fatal error, we'll be leaving the stream
            // read pointer in an unknown position.
            throw ParserException(ss.str()); 
        }
    }
}


const BitmapInfo* 
fill_style::get_bitmap_info(Renderer& renderer) const 
{    
    assert(m_type != SWF::FILL_SOLID);

    switch (m_type)
    {
        case SWF::FILL_TILED_BITMAP:
        case SWF::FILL_CLIPPED_BITMAP:
        case SWF::FILL_TILED_BITMAP_HARD:
        case SWF::FILL_CLIPPED_BITMAP_HARD:
            return boost::get<BitmapFill>(_fill).bitmapInfo.get();
   
        case SWF::FILL_LINEAR_GRADIENT:
        case SWF::FILL_RADIAL_GRADIENT:
            return need_gradient_bitmap(renderer);
        default:
            log_error(_("Unknown fill style %d"), m_type);
            // Seems a bit drastic...
            std::abort();
    }
}

const SWFMatrix&
fill_style::getBitmapMatrix() const 
{
    return boost::get<BitmapFill>(_fill).matrix;
}

const SWFMatrix&
fill_style::getGradientMatrix() const 
{
    // TODO: Why do we separate bitmap and gradient matrices? 
    return boost::apply_visitor(ConstMatrix(), _fill);
}

const BitmapInfo*
GradientFill::createBitmap(Renderer& renderer) const
{
    
    std::auto_ptr<ImageRGBA> im;

    switch (type)
    {
        case LINEAR:
            // Linear gradient.
            im.reset(new ImageRGBA(256, 1));

            for (size_t i = 0; i < im->width(); i++) {

                rgba sample = sampleGradient(*this, i);
                im->setPixel(i, 0, sample.m_r, sample.m_g,
                        sample.m_b, sample.m_a);
            }
            break;

        case GradientFill::RADIAL:
            // Radial gradient.
            im.reset(new ImageRGBA(64, 64));

            for (size_t j = 0; j < im->height(); j++) {
                for (size_t i = 0; i < im->width(); i++) {
                    float radius = (im->height() - 1) / 2.0f;
                    float y = (j - radius) / radius;
                    float x = (i - radius) / radius;
                    int ratio = std::floor(255.5f * std::sqrt(x * x + y * y));
                    if (ratio > 255) {
                        ratio = 255;
                    }
                    rgba sample = sampleGradient(*this, ratio);
                    im->setPixel(i, j, sample.m_r, sample.m_g,
                            sample.m_b, sample.m_a);
                }
            }
            break;

        case GradientFill::FOCAL:
            // Focal gradient.
            im.reset(new ImageRGBA(64, 64));

            for (size_t j = 0; j < im->height(); j++)
            {
                for (size_t i = 0; i < im->width(); i++)
                {
                    float radiusy = (im->height() - 1) / 2.0f;
                    float radiusx = radiusy + std::abs(radiusy * focalPoint);
                    float y = (j - radiusy) / radiusy;
                    float x = (i - radiusx) / radiusx;
                    int ratio = std::floor(255.5f * std::sqrt(x*x + y*y));
                    
                    if (ratio > 255) ratio = 255;

                    rgba sample = sampleGradient(*this, ratio);
                    im->setPixel(i, j, sample.m_r, sample.m_g,
                            sample.m_b, sample.m_a);
                }
            }
            break;
        default:
            break;
    }

    const BitmapInfo* bi = renderer.createBitmapInfo(
                    static_cast<std::auto_ptr<GnashImage> >(im));

    return bi;
}


const BitmapInfo*
fill_style::need_gradient_bitmap(Renderer& renderer) const 
{
    GradientFill& gf = const_cast<GradientFill&>(
            boost::get<GradientFill>(_fill));

    if (!gf.gradientBitmap) {
        gf.gradientBitmap = gf.createBitmap(renderer);
    }
    return gf.gradientBitmap.get();

}

// Sets this style to a blend of a and b.  t = [0,1]
void
fill_style::set_lerp(const fill_style& a, const fill_style& b, float t)
{
    assert(t >= 0 && t <= 1);

    m_type = a.get_type();
    
    // TODO: set type better
    _fill = a._fill;
    assert(m_type == b.get_type());
    boost::apply_visitor(SetLerp(a._fill, b._fill, t), _fill);

}

size_t
fill_style::get_color_stop_count() const 
{
    return boost::get<GradientFill>(_fill).gradients.size();
}

const gradient_record& 
fill_style::get_color_stop(size_t index) const
{
    const GradientFill& gf = boost::get<GradientFill>(_fill);
    assert(index < gf.gradients.size());
    return gf.gradients[index];
}

fill_style::fill_style(const BitmapInfo* const bitmap, const SWFMatrix& mat)
    :
    _fill(BitmapFill()),
    m_type(SWF::FILL_CLIPPED_BITMAP)
{
    boost::get<BitmapFill>(_fill).matrix = mat;
    boost::get<BitmapFill>(_fill).bitmapInfo = bitmap;
}

void
fill_style::setSolid(const rgba& color)
{
    m_type = SWF::FILL_SOLID;
    _fill = SolidFill(color);
}

void
fill_style::setLinearGradient(const std::vector<gradient_record>& gradients,
        const SWFMatrix& mat)
{

    assert(!gradients.empty());
    
    // We must ensure that all gradients have more than one colour stop
    // because asking renderers to render a gradient with one colour
    // can cause them to invoke UB.
    if (gradients.size() < 2) {
        setSolid(gradients[0].m_color);
        return;
    }

    m_type = SWF::FILL_LINEAR_GRADIENT;

    GradientFill gf(GradientFill::LINEAR);

    gf.gradients = gradients;

    gf.matrix = mat;
    gf.gradientBitmap = 0;
    _fill = gf;
}

void
fill_style::setRadialGradient(const std::vector<gradient_record>& gradients,
        const SWFMatrix& mat)
{
    assert(!gradients.empty());
    
    // We must ensure that all gradients have more than one colour stop
    // because asking renderers to render a gradient with one colour
    // can cause them to invoke UB.
    if (gradients.size() < 2) {
        setSolid(gradients[0].m_color);
        return;
    }
    
    m_type = SWF::FILL_RADIAL_GRADIENT;

    GradientFill gf(GradientFill::RADIAL);
    gf.gradients = gradients;

    gf.matrix = mat;
    gf.gradientBitmap = 0;
    _fill = gf;
}

namespace {

rgba
sampleGradient(const GradientFill& fill, boost::uint8_t ratio)
{

    const std::vector<gradient_record>& r = fill.gradients;

    if (r.empty()) {
        static const rgba black;
        return black;
    }

    // By specs, first gradient should *always* be 0, 
    // anyway a malformed SWF could break this,
    // so we cannot rely on that information...
    if (ratio < r[0].m_ratio) {
        IF_VERBOSE_MALFORMED_SWF(
            LOG_ONCE(log_swferror(_("First gradient in a fill_style "
                    "has position==%d (expected 0). This seems to be common, "
                    "so will warn only once."), +r[0].m_ratio);
            );
        );
        return r[0].m_color;
    }

    if (ratio >= r.back().m_ratio) {
        return r.back().m_color;
    }
        
    for (size_t i = 1, n = r.size(); i < n; ++i) {

        const gradient_record& gr1 = r[i];
        if (gr1.m_ratio < ratio) continue;

        const gradient_record& gr0 = r[i - 1];
        if (gr0.m_ratio > ratio) continue;

        float f = 0.0f;

        if (gr0.m_ratio != gr1.m_ratio) {
            f = (ratio - gr0.m_ratio) / float(gr1.m_ratio - gr0.m_ratio);
        }
        else {
            // Ratios are equal IFF first and second gradient_record
            // have the same ratio. This would be a malformed SWF.
            IF_VERBOSE_MALFORMED_SWF(
                log_swferror(_("two gradients in a fill_style "
                    "have the same position/ratio: %d"),
                    gr0.m_ratio);
            );
        }

        rgba result;
        result.set_lerp(gr0.m_color, gr1.m_color, f);
        return result;
    }

    // Assuming gradients are ordered by m_ratio? see start comment
    return r.back().m_color;
}

}

std::ostream&
operator<<(std::ostream& os, const BitmapFill::SmoothingPolicy& p)
{
    switch (p) {
        case BitmapFill::SMOOTHING_UNSPECIFIED:
            os << "unspecified";
            break;
        case BitmapFill::SMOOTHING_ON:
            os << "on";
            break;
        case BitmapFill::SMOOTHING_OFF:
            os << "off";
            break;
        default:
            // cast to int required to avoid infinite recursion
            os << "unknown " << +p;
            break;
    }
    return os;
}

} // end of namespace


// Local Variables:
// mode: C++
// End:
