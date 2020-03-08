/*****************************************************************************/
/* The contents of this file are subject to the Mozilla Public License       */
/* Version 1.0 (the "License"); you may not use this file except in          */
/* compliance with the License.  You may obtain a copy of the License at     */
/* http://www.mozilla.org/MPL/                                               */
/*                                                                           */
/* Software distributed under the License is distributed on an "AS IS"       */
/* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the  */
/* License for the specific language governing rights and limitations under  */
/* the License.                                                              */
/*                                                                           */
/* The Original Code is the Open H323 Library.                               */
/*                                                                           */
/* The Initial Developer of the Original Code is Matthias Schneider          */
/* Copyright (C) 2007 Matthias Schneider, All Rights Reserved.               */
/*                                                                           */
/* Contributor(s): Matthias Schneider (ma30002000@yahoo.de)                  */
/*                                                                           */
/* Alternatively, the contents of this file may be used under the terms of   */
/* the GNU General Public License Version 2 or later (the "GPL"), in which   */
/* case the provisions of the GPL are applicable instead of those above.  If */
/* you wish to allow use of your version of this file only under the terms   */
/* of the GPL and not to allow others to use your version of this file under */
/* the MPL, indicate your decision by deleting the provisions above and      */
/* replace them with the notice and other provisions required by the GPL.    */
/* If you do not delete the provisions above, a recipient may use your       */

#ifndef __H265FRAME_H__
#define __H265FRAME_H__

#include <stdint.h>
#include <iostream>
#include <string.h>
#include "RTSP/RTSP.h"
#include "Base/Base.h"
using namespace Public::Base;
using namespace Public::RTSP;

#define AV_RB16(x) ((((const uint8_t*)(x))[0] << 8)|((const uint8_t*)(x))[1])

class H265Frame
{
public:
	shared_ptr<RTPFrame>	m_frame;
	int m_firstfrag = 0;
	int	m_lastfrag = 0;
public:
  H265Frame();
  ~H265Frame();
  int handleHevcFrame(const shared_ptr<STREAM_TRANS_INFO>& transinfo, const shared_ptr<RTPPackage>& rtp);
  void resetBuffer();
private:
  int handleHevcRtpPackage(uint32_t timestamp,const uint8_t *rtpbuf, int rtplen);
  void handleFragPackage(const uint8_t *rtpbuf, int rtplen,int start_bit,const uint8_t *nal_header,int nal_header_len);
  int handleAggregatedPacket(const uint8_t *rtpbuf, int rtplen);
};

#endif /* __H265FRAME_H__ */
