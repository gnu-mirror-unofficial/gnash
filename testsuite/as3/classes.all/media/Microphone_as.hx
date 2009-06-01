// Microphone_as.hx:  ActionScript 3 "Microphone" class, for Gnash.
//
// This test is only valid for Flash v9 and higher.
//
// Generated by gen-as3.sh on: 20090503 by "rob". Remove this
// after any hand editing loosing changes.
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

// This test case must be processed by CPP before compiling to include the
//  DejaGnu.hx header file for the testing framework support.

#if flash9
import flash.media.Microphone;
import flash.media.SoundTransform;
import flash.display.MovieClip;
import flash.Lib;
import Type;
#end

import DejaGnu;

// Class must be named with the PP prefix, as that's the name the
// file passed to haxe will have after the preprocessing step
class Microphone_as {
    static function main() {
        // MicroPhone doesn't have a constructor, instead it must be
        // accessed through a static method.
        var x1:Microphone = Microphone.getMicrophone();

        // Make sure we actually get a valid class        
        if (x1 != null) {
            DejaGnu.pass("Microphone class exists");
        } else {
            DejaGnu.fail("Microphone lass doesn't exist");
        }
// Tests to see if all the properties exist. All these do is test for
// existance of a property, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (Type.typeof(x1.activityLevel) == ValueType.TFloat) {
	    DejaGnu.pass("Microphone::activityLevel property exists");
	} else {
	    DejaGnu.fail("Microphone::activityLevel property doesn't exist");
	}
	
	//FIXME: this will need to be implemented if speex codec is supported
	//FIXME: it would be nice if this analyzed the string to see if it is
	//a supported codec
	//if (Type.typeof(x1.codec) == ValueType.TObject)  {
	//	DejaGnu.pass("Microphone::codec property exists");
	//} else {
	//	DejaGnu.fail("Microphone::codec property doesn't exist");
	//}
	//if (Type.typeof(x1.encodeQuality) == ValueType.TInt)  {
	//	DejaGnu.pass("Microphone::encodeQuality (speex) property exists");
	//} else {
	//	DejaGnu.fail("Microphone::encodeQuality (speex) property doesn't exist");
	//}
	//if (Type.typeof(x1.framesPerPacket) == ValueType.TInt) {
	//	DejaGnu.pass("Microphone::framesPerPacket (speex) property exists");
	//} else {
	//	DejaGnu.fail("Microphone::framesPerPacket (speex) property doesn't exist");
	//}
	
	
	if (Type.typeof(x1.gain) == ValueType.TFloat) {
	    DejaGnu.pass("Microphone::gain property exists");
	} else {
	    DejaGnu.fail("Microphone::gain property doesn't exist");
	}
	if (Type.typeof(x1.index) == ValueType.TInt) {
	    DejaGnu.pass("Microphone::index property exists");
	} else {
	    DejaGnu.fail("Microphone::index property doesn't exist");
	}
	if (x1.muted == (false || true)) {
	    DejaGnu.pass("Microphone::muted property exists");
	} else {
	    DejaGnu.fail("Microphone::muted property doesn't exist");
	}
	//FIXME: it would be nice if this parsed and checked the name string
	if (Type.typeof(x1.name) == ValueType.TObject) {
	    DejaGnu.pass("Microphone::name property exists");
	} else {
	    DejaGnu.fail("Microphone::name property doesn't exist");
	}
// FIXME: this returns an array
// 	if (x1.names == 0) {
// 	    DejaGnu.pass("Microphone::names property exists");
// 	} else {
// 	    DejaGnu.fail("Microphone::names property doesn't exist");
// 	}
	if (Type.typeof(x1.rate) == ValueType.TInt) {
	    DejaGnu.pass("Microphone::rate property exists");
	} else {
	    DejaGnu.fail("Microphone::rate property doesn't exist");
	}
	if (Type.typeof(x1.silenceLevel) == ValueType.TFloat) {
	    DejaGnu.pass("Microphone::silenceLevel property exists");
	} else {
	    DejaGnu.fail("Microphone::silenceLevel property doesn't exist");
	}
	if (Type.typeof(x1.silenceTimeout) == ValueType.TInt) {
	    DejaGnu.pass("Microphone::silenceTimeout property exists");
	} else {
	    DejaGnu.fail("Microphone::silenceTimeout property doesn't exist");
	}
	if (Type.typeof(x1.soundTransform) == Type.typeof(SoundTransform)) {
	    DejaGnu.pass("Microphone::soundTransform property exists");
	} else {
	    DejaGnu.fail("Microphone::soundTransform property doesn't exist");
	}
	if (x1.useEchoSuppression == (true || false)) {
	    DejaGnu.pass("Microphone::useEchoSuppression property exists");
	} else {
	    DejaGnu.fail("Microphone::useEchoSuppression property doesn't exist");
	}

// Tests to see if all the methods exist. All these do is test for
// existance of a method, and don't test the functionality at all. This
// is primarily useful only to test completeness of the API implementation.
	if (Type.typeof(x1.setLoopBack) == ValueType.TFunction) {
	    DejaGnu.pass("Microphone::setLoopBack() method exists");
	} else {
	    DejaGnu.fail("Microphone::setLoopBack() method doesn't exist");
	}
	if (Type.typeof(x1.setSilenceLevel) == ValueType.TFunction) {
	    DejaGnu.pass("Microphone::setSilenceLevel() method exists");
	} else {
	    DejaGnu.fail("Microphone::setSilenceLevel() method doesn't exist");
	}
	if (Type.typeof(x1.setUseEchoSuppression) == ValueType.TFunction) {
	    DejaGnu.pass("Microphone::setUseEchoSuppression() method exists");
	} else {
	    DejaGnu.fail("Microphone::setUseEchoSuppression() method doesn't exist");
	}

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}
