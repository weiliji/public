#
#  Copyright (c)1998-2012, Chongqing Public Technology
#  All Rights Reserved.
#
#	 Description:
#  $Id: option.mk 3877 2015-03-02 10:28:16Z shaoyikai $
#

ifeq ($(strip ${GENSVNVER}), YES)
	CFLAGS += -DSVNVER
endif

