/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2018 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version, with the additional exemption that
 * compiling, linking, and/or using OpenSSL is allowed.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * A copy of the GNU Lesser General Public License is in the file COPYING.
 */

#include <memory.h>
#include <cnl-cpp/segmented-object-handler.hpp>

using namespace std;
using namespace ndn;
using namespace ndn::func_lib;

namespace cnl_cpp {

SegmentedObjectHandler::Impl::Impl
  (SegmentedObjectHandler& outerHandler,
   const OnSegmentedObject& onSegmentedObject)
: outerHandler_(outerHandler), finished_(false), totalSize_(0),
  onSegmentedObject_(onSegmentedObject)
{
}

void
SegmentedObjectHandler::Impl::initialize()
{
  outerHandler_.addOnSegment
    (bind(&SegmentedObjectHandler::Impl::onSegment, shared_from_this(), _1, _2, _3));
}

void
SegmentedObjectHandler::Impl::onSegment
  (SegmentStreamHandler& handler, Namespace* segmentNamespace,
   uint64_t callbackId)
{
  if (finished_)
    // We already finished and called onContent. (We don't expect this.)
    return;

  if (segmentNamespace) {
    segments_.push_back(segmentNamespace->getBlobObject());
    totalSize_ += segmentNamespace->getBlobObject().size();
  }
  else {
      // Concatenate the segments.
      ptr_lib::shared_ptr<vector<uint8_t> > content =
        ptr_lib::make_shared<vector<uint8_t> >(totalSize_);
      size_t offset = 0;
      for (size_t i = 0; i < segments_.size(); ++i) {
        const Blob& segment = segments_[i];
        memcpy(&(*content)[offset], segment.buf(), segment.size());
        offset += segment.size();
        // Free the memory.
        segments_[i] = Blob();
      }

      // Free memory.
      segments_.clear();
      finished_ = true;

      Blob contentBlob = Blob(content, false);
      // Debug: Fix this hack. How can we attach content to a namespace
      // node which has no associated Data packet? Who is authorized to do so?
      outerHandler_.getNamespace().debugOnContentTransformed
        (ptr_lib::make_shared<Data>(), 
         ptr_lib::make_shared<BlobObject>(contentBlob));

      if (onSegmentedObject_)
        onSegmentedObject_(outerHandler_, contentBlob);
  }
}

}