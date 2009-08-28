// Camera_as.cpp:  ActionScript "Camera" class, for Gnash.
//
//   Copyright (C) 2009 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include "flash/media/Camera_as.h"
#include "as_object.h" // for inheritance
#include "log.h"
#include "fn_call.h"
#include "Global_as.h"
#include "smart_ptr.h" 
#include "builtin_function.h" 
#include "NativeFunction.h" 
#include "Object.h" 
#include "Array_as.h"
#include "MediaHandler.h"
#include "VideoInput.h"

#include <sstream>


namespace gnash {

as_value camera_get(const fn_call& fn);
as_value camera_getCamera(const fn_call& fn);
as_value camera_setmode(const fn_call& fn);
as_value camera_setmotionlevel(const fn_call& fn);
as_value camera_setquality(const fn_call& fn);
as_value camera_setLoopback(const fn_call& fn);
as_value camera_setCursor(const fn_call& fn);
as_value camera_setKeyFrameInterval(const fn_call& fn);

as_value camera_activitylevel(const fn_call& fn);
as_value camera_bandwidth(const fn_call& fn);
as_value camera_currentFps(const fn_call& fn);
as_value camera_fps(const fn_call& fn);
as_value camera_height(const fn_call& fn);
as_value camera_index(const fn_call& fn);
as_value camera_motionLevel(const fn_call& fn);
as_value camera_motionTimeout(const fn_call& fn);
as_value camera_muted(const fn_call& fn);
as_value camera_name(const fn_call& fn);
as_value camera_names(const fn_call& fn);
as_value camera_quality(const fn_call& fn);
as_value camera_width(const fn_call& fn);


void
attachCameraStaticInterface(as_object& o)
{
    Global_as* gl = getGlobal(o);
    
    const int flags = 0;

	o.init_member("get", gl->createFunction(camera_get), flags);

    VM& vm = getVM(o);
    NativeFunction* getset = vm.getNative(2102, 201);
    o.init_property("names", *getset, *getset);

}

void
attachCameraAS3StaticInterface(as_object& o)
{
    Global_as* gl = getGlobal(o);
    o.init_member("getCamera", gl->createFunction(camera_getCamera));
}

static void
attachCameraInterface(as_object& o)
{
    
    const int flags = as_object::DefaultFlags | PropFlags::onlySWF6Up;

    VM& vm = getVM(o);
    o.init_member("setMode", vm.getNative(2102, 0), flags);
    o.init_member("setQuality", vm.getNative(2102, 1), flags);
    o.init_member("setKeyFrameInterval", vm.getNative(2102, 2), flags);
    o.init_member("setMotionLevel", vm.getNative(2102, 3), flags);
    o.init_member("setLoopback", vm.getNative(2102, 4), flags);
    o.init_member("setCursor", vm.getNative(2102, 5), flags);

}

// Properties attached to the prototype when Camera.get() is called
void
attachCameraProperties(as_object& o)
{
    Global_as* gl = getGlobal(o);
    boost::intrusive_ptr<builtin_function> getset;

    getset = gl->createFunction(camera_activitylevel);
    o.init_property("activityLevel", *getset, *getset);
    getset = gl->createFunction(camera_bandwidth);
    o.init_property("bandwidth", *getset, *getset);
    getset = gl->createFunction(camera_currentFps);
    o.init_property("currentFps", *getset, *getset);
    getset = gl->createFunction(camera_fps);
    o.init_property("fps", *getset, *getset);
    getset = gl->createFunction(camera_height);
    o.init_property("height", *getset, *getset);
    getset = gl->createFunction(camera_index);
    o.init_property("index", *getset, *getset);
    getset = gl->createFunction(camera_motionLevel);
    o.init_property("motionLevel", *getset, *getset);
    getset = gl->createFunction(camera_motionTimeout);
    o.init_property("motionTimeout", *getset, *getset);
    getset = gl->createFunction(camera_muted);
    o.init_property("muted", *getset, *getset);
    getset = gl->createFunction(camera_name);
    o.init_property("name", *getset, *getset);
    getset = gl->createFunction(camera_quality);
    o.init_property("quality", *getset, *getset);
    getset = gl->createFunction(camera_width);
    o.init_property("width", *getset, *getset);
}

static as_object*
getCameraInterface()
{
    static boost::intrusive_ptr<as_object> o;
    if ( ! o )
    {
        o = new as_object(getObjectInterface());
        attachCameraInterface(*o);
    }
    return o.get();
}

class Camera_as: public as_object
{
public:

    Camera_as(media::VideoInput* input)
        :
        as_object(getCameraInterface()),
        _input(input),
        _loopback(false)
    {
        assert(input);
    }

    bool muted() const {
        return _input->muted();
    }

    size_t width() const {
        return _input->width();
    }

    size_t height() const {
        return _input->height();
    }

    double fps() const {
        return _input->fps();
    }

    double currentFPS() const {
        return _input->currentFPS();
    }

    double activityLevel() const {
        return _input->activityLevel();
    }

    double bandwidth() const {
        return _input->bandwidth();
    }

    size_t index() const {
        return _input->index();
    }

    void setMode(size_t width, size_t height, double fps, bool favorArea) {
        _input->requestMode(width, height, fps, favorArea);
    }

    void setMotionLevel(size_t level, double timeout) {
        _input->setMotionLevel(level);
        _input->setMotionTimeout(timeout);
    }

    double motionLevel() const {
        return _input->motionLevel();
    }

    double motionTimeout() const {
        return _input->motionTimeout();
    }

    const std::string& name() const {
        return _input->name();
    }

    size_t quality() const {
        return _input->quality();
    }

    void setQuality(double bandwidth, size_t quality) {
        _input->setBandwidth(bandwidth);
        _input->setQuality(quality);
    }

    void setLoopback(bool b) {
        _loopback = b;
    }

private:

    media::VideoInput* _input;

    // TODO: see whether this should be handled in the VideoInput class
    bool _loopback;
};

// AS2 static accessor.
as_value
camera_get(const fn_call& fn)
{

    // Properties are attached to the prototype when get() is called.
    as_object* proto = getCameraInterface();

    // This is an AS2-only function, so don't worry about VM version.
    attachCameraProperties(*proto);

    // TODO: this should return the same object when the same device is
    // meant, not a new object each time. It will be necessary to query
    // the MediaHandler for this, and possibly to store the as_objects
    // somewhere.
    //
    media::MediaHandler* handler = media::MediaHandler::get();
    if (!handler) {
        log_error(_("No MediaHandler exists! Cannot create a Camera object"));
        return as_value();
    }
    media::VideoInput* input = handler->getVideoInput(0);

    if (!input) {
        // TODO: what should happen if the index is not available?
        return as_value();
    }

    boost::intrusive_ptr<as_object> obj = new Camera_as(input); 

    const size_t nargs = fn.nargs;
    if (nargs > 0) {
        log_debug("%s: the camera is automatically chosen from gnashrc",
                "Camera.get()");
    }
    return as_value(obj.get()); 
}

// AS3 static accessor.
as_value
camera_getCamera(const fn_call& fn)
{
    media::VideoInput* input = media::MediaHandler::get()->getVideoInput(0);
    
    boost::intrusive_ptr<as_object> obj = new Camera_as(input);
    
    int numargs = fn.nargs;
    if (numargs > 0) {
        log_debug("%s: the camera is automatically chosen from gnashrc", __FUNCTION__);
    }
    return as_value(obj.get()); // will keep alive
}

as_value
camera_setmode(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);
    
    const size_t nargs = fn.nargs;

    const double width = nargs ? fn.arg(0).to_number() : 160;
    const double height = nargs > 1 ? fn.arg(1).to_number() : 120;
    const double fps = nargs >  2? fn.arg(2).to_number() : 15;
    const bool favorArea = nargs > 3 ? fn.arg(3).to_bool() : true;

    // TODO: handle overflow
    const size_t reqWidth = std::max<double>(width, 0);
    const size_t reqHeight = std::max<double>(height, 0);

    ptr->setMode(reqWidth, reqHeight, fps, favorArea);

    return as_value();
}

as_value
camera_setmotionlevel(const fn_call& fn)
{
    log_unimpl ("Camera::motionLevel can be set, but it's not implemented");
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>
        (fn.this_ptr);
    
    const size_t nargs = fn.nargs;

    const double ml = nargs > 0 ? fn.arg(0).to_number() : 50;
    const double mt = nargs > 1 ? fn.arg(1).to_number() : 2000;

    const size_t motionLevel = (ml >= 0 && ml <= 100) ? ml : 100;

    ptr->setMotionLevel(motionLevel, mt);

    return as_value();
}


as_value
camera_setquality(const fn_call& fn)
{
    log_unimpl ("Camera::quality can be set, but it's not implemented");
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>
        (fn.this_ptr);

    const size_t nargs = fn.nargs;

    const double b = nargs > 0 ? fn.arg(0).to_number() : 16384;
    const double q = nargs > 1 ? fn.arg(1).to_number() : 0;

    size_t quality = (q < 0 || q > 100) ? 100 : q;

    ptr->setQuality(b, quality);

    return as_value();
}


as_value
camera_activitylevel(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera::activityLevel only has default value");
        return as_value(ptr->activityLevel());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set activity property of Camera"));
    );

    return as_value();
}

as_value
camera_bandwidth(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera::bandwidth only has default value");
        return as_value(ptr->bandwidth());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set bandwidth property of Camera"));
    );

    return as_value();
}

as_value
camera_currentFps(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        return as_value(ptr->currentFPS());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set currentFPS property of Camera"));
    );

    return as_value();
}

as_value
camera_fps(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        return as_value(ptr->fps());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set fps property of Camera"));
    );

    return as_value();
}

as_value
camera_height(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        return as_value(ptr->height());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set height property of Camera, use setMode"));
    );

    return as_value();
}

as_value
camera_index(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) 
    {
        // livedocs say that this function should return an integer,
        // but in testing the pp returns the value as a string
        int value = ptr->index();
        
        std::ostringstream ss;
        ss << value;
        return as_value(ss.str());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set index property of Camera"));
    );

    return as_value();
}

as_value
camera_motionLevel(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera::motionLevel only has default value");
        return as_value(ptr->motionLevel());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set motionLevel property of Camera"));
    );

    return as_value();
}

as_value
camera_motionTimeout(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera::motionTimeout");
        return as_value(ptr->motionTimeout());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set motionTimeout property of Camera"));
    );

    return as_value();
}

as_value
camera_muted(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera.muted");
        return as_value(ptr->muted());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set muted property of Camera"));
    );

    return as_value();
}

as_value
camera_name(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        return as_value(ptr->name());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set name property of Camera"));
    );

    return as_value();
}

as_value
camera_names(const fn_call& fn)
{
    if (fn.nargs) {
        IF_VERBOSE_ASCODING_ERRORS(
            log_aserror(_("Attempt to set names property of Camera"));
        );
        return as_value();
    }

    std::vector<std::string> names;
    media::MediaHandler::get()->cameraNames(names);
    
    const size_t size = names.size();
    
    boost::intrusive_ptr<Array_as> data = new Array_as;

    for (size_t i = 0; i < size; ++i) {
        data->push(names[i]);
    }
    
    return as_value(data.get());
} 


as_value
camera_quality(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        log_unimpl("Camera::quality has only default values");
        return as_value(ptr->quality());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set quality property of Camera"));
    );

    return as_value();
}

as_value
camera_new(const fn_call& /*fn*/)
{
    return as_value();
}

as_value
camera_setLoopback(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr = ensureType<Camera_as>(fn.this_ptr);
    
    if (!fn.nargs) {
        // TODO: log AS error.
        return as_value();
    }

    if (fn.nargs > 1) {
        log_aserror("%s: Too many arguments", "Camera.setLoopback");
    }

    ptr->setLoopback(fn.arg(0).to_bool());
    
    return as_value();
}

as_value
camera_setCursor(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("Camera.setCursor"));
    return as_value();
}

as_value
camera_setKeyFrameInterval(const fn_call& /*fn*/)
{
    LOG_ONCE(log_unimpl("Camera.setKeyFrameInterval"));
    return as_value();
}

as_value
camera_width(const fn_call& fn)
{
    boost::intrusive_ptr<Camera_as> ptr =
        ensureType<Camera_as>(fn.this_ptr);

    if (!fn.nargs) {
        return as_value(ptr->width());
    }

    IF_VERBOSE_ASCODING_ERRORS(
        log_aserror(_("Attempt to set width property of Camera, use setMode"));
    );

    return as_value();
}


// extern (used by Global.cpp)
void
camera_class_init(as_object& where, const ObjectURI& uri)
{

    Global_as* gl = getGlobal(where);
    
    as_object* proto = getCameraInterface();
    
    // This is going to be the global Camera "class"/"function"
    as_object* cl;

    //for versions lower than 8, the ctor call was get(), for 9 and higher
    //the ctor was getCamera()
    if (isAS3(getVM(where))) {
        cl = gl->createClass(&camera_new, proto);
        attachCameraAS3StaticInterface(*cl);
    } else {
        cl = gl->createClass(&camera_new, proto);
        attachCameraStaticInterface(*cl);
    }
    
    // Register _global.Camera
    where.init_member(getName(uri), cl, as_object::DefaultFlags,
            getNamespace(uri));

}

void
registerCameraNative(as_object& global)
{
    VM& vm = getVM(global);
    vm.registerNative(camera_names, 2102, 201);
    vm.registerNative(camera_setmode, 2102, 0);
    vm.registerNative(camera_setquality, 2102, 1);
    vm.registerNative(camera_setKeyFrameInterval, 2102, 2);
    vm.registerNative(camera_setmotionlevel, 2102, 3);
    vm.registerNative(camera_setLoopback, 2102, 4);
    vm.registerNative(camera_setCursor, 2102, 5);
}

} // end of gnash namespace

