AC_PLUGIN_DIR=ptlib/codecs/audio
prefix=/usr/local
ifndef prefix
prefix=/usr/local
endif
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
target_os=linux-gnu

SONAME	= opus
SRCDIR	= ./src/opus ./src/celt ./src/silk ./src/silk/float
OBJDIR	= ./obj
PLUGINDIR=../..

CC		=gcc -std=gnu99
CXX		=g++
LDSO		=-shared
PLUGINEXT	=so
STDCCFLAGS	+=-fPIC -g
EXTRACFLAGS     =-Isrc/include -Iscr/opus -Isrc/celt -Isrc/silk -Isrc/silk/float
#WARNINGS	=-Wall -W -Wstrict-prototypes -Wextra -Wcast-align -Wnested-externs -Wshadow
CFLAGS		=-O2 -g $(WARNINGS) -DOPUS_BUILD -DVAR_ARRAYS
LDFLAGS  	=

SRCS	= \
\
opuscodec.cxx \
\
src/opus/opus.c \
src/opus/opus_compare.c \
src/opus/opus_decoder.c \
src/opus/opus_encoder.c \
src/opus/opus_multistream.c \
src/opus/repacketizer.c \
\
src/celt/bands.c \
src/celt/celt.c \
src/celt/celt_lpc.c \
src/celt/cwrs.c \
src/celt/entcode.c \
src/celt/entdec.c \
src/celt/entenc.c \
src/celt/kiss_fft.c \
src/celt/laplace.c \
src/celt/mathops.c \
src/celt/mdct.c \
src/celt/modes.c \
src/celt/pitch.c \
src/celt/quant_bands.c \
src/celt/rate.c \
src/celt/vq.c \
\
src/silk/CNG.c \
src/silk/code_signs.c \
src/silk/init_decoder.c \
src/silk/decode_core.c \
src/silk/decode_frame.c \
src/silk/decode_parameters.c \
src/silk/decode_indices.c \
src/silk/decode_pulses.c \
src/silk/decoder_set_fs.c \
src/silk/dec_API.c \
src/silk/enc_API.c \
src/silk/encode_indices.c \
src/silk/encode_pulses.c \
src/silk/gain_quant.c \
src/silk/interpolate.c \
src/silk/LP_variable_cutoff.c \
src/silk/NLSF_decode.c \
src/silk/NSQ.c \
src/silk/NSQ_del_dec.c \
src/silk/PLC.c \
src/silk/shell_coder.c \
src/silk/tables_gain.c \
src/silk/tables_LTP.c \
src/silk/tables_NLSF_CB_NB_MB.c \
src/silk/tables_NLSF_CB_WB.c \
src/silk/tables_other.c \
src/silk/tables_pitch_lag.c \
src/silk/tables_pulses_per_block.c \
src/silk/VAD.c \
src/silk/control_audio_bandwidth.c \
src/silk/quant_LTP_gains.c \
src/silk/VQ_WMat_EC.c \
src/silk/HP_variable_cutoff.c \
src/silk/NLSF_encode.c \
src/silk/NLSF_VQ.c \
src/silk/NLSF_unpack.c \
src/silk/NLSF_del_dec_quant.c \
src/silk/process_NLSFs.c \
src/silk/stereo_LR_to_MS.c \
src/silk/stereo_MS_to_LR.c \
src/silk/check_control_input.c \
src/silk/control_SNR.c \
src/silk/init_encoder.c \
src/silk/control_codec.c \
src/silk/A2NLSF.c \
src/silk/ana_filt_bank_1.c \
src/silk/biquad_alt.c \
src/silk/bwexpander_32.c \
src/silk/bwexpander.c \
src/silk/debug.c \
src/silk/decode_pitch.c \
src/silk/inner_prod_aligned.c \
src/silk/lin2log.c \
src/silk/log2lin.c \
src/silk/LPC_analysis_filter.c \
src/silk/LPC_inv_pred_gain.c \
src/silk/table_LSF_cos.c \
src/silk/NLSF2A.c \
src/silk/NLSF_stabilize.c \
src/silk/NLSF_VQ_weights_laroia.c \
src/silk/pitch_est_tables.c \
src/silk/resampler.c \
src/silk/resampler_down2_3.c \
src/silk/resampler_down2.c \
src/silk/resampler_private_AR2.c \
src/silk/resampler_private_down_FIR.c \
src/silk/resampler_private_IIR_FIR.c \
src/silk/resampler_private_up2_HQ.c \
src/silk/resampler_rom.c \
src/silk/sigm_Q15.c \
src/silk/sort.c \
src/silk/sum_sqr_shift.c \
src/silk/stereo_decode_pred.c \
src/silk/stereo_encode_pred.c \
src/silk/stereo_find_predictor.c \
src/silk/stereo_quant_pred.c \
\
src/silk/float/apply_sine_window_FLP.c \
src/silk/float/corrMatrix_FLP.c \
src/silk/float/encode_frame_FLP.c \
src/silk/float/find_LPC_FLP.c \
src/silk/float/find_LTP_FLP.c \
src/silk/float/find_pitch_lags_FLP.c \
src/silk/float/find_pred_coefs_FLP.c \
src/silk/float/LPC_analysis_filter_FLP.c \
src/silk/float/LTP_analysis_filter_FLP.c \
src/silk/float/LTP_scale_ctrl_FLP.c \
src/silk/float/noise_shape_analysis_FLP.c \
src/silk/float/prefilter_FLP.c \
src/silk/float/process_gains_FLP.c \
src/silk/float/regularize_correlations_FLP.c \
src/silk/float/residual_energy_FLP.c \
src/silk/float/solve_LS_FLP.c \
src/silk/float/warped_autocorrelation_FLP.c \
src/silk/float/wrappers_FLP.c \
src/silk/float/autocorrelation_FLP.c \
src/silk/float/burg_modified_FLP.c \
src/silk/float/bwexpander_FLP.c \
src/silk/float/energy_FLP.c \
src/silk/float/inner_product_FLP.c \
src/silk/float/k2a_FLP.c \
src/silk/float/levinsondurbin_FLP.c \
src/silk/float/LPC_inv_pred_gain_FLP.c \
src/silk/float/pitch_analysis_core_FLP.c \
src/silk/float/scale_copy_vector_FLP.c \
src/silk/float/scale_vector_FLP.c \
src/silk/float/schur_FLP.c \
src/silk/float/sort_FLP.c


vpath	%.o $(OBJDIR)
vpath	%.c $(SRCDIR)
vpath	%.cxx .

ifeq ($(VERBOSE),)
Q_CC = @echo [CC] `echo $< | sed s^@OPALDIR@/^^` ;
Q_LD = @echo [LD] `echo $(PLUGIN) | sed s^@OPALDIR@/^^` ;
endif


$(OBJDIR)/%.o : %.c
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(Q_CC)$(CC) -I../../../include $(STDCCFLAGS) $(OPTCCFLAGS) $(EXTRACFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(OBJDIR) >/dev/null 2>&1
	$(CXX) $(EXTRACCFLAGS) $(STDCCFLAGS) $(OPTCCFLAGS) $(CFLAGS) -c $< -o $@

PLUGIN	= ./$(SONAME)_audio_pwplugin.$(PLUGINEXT)

OBJECTS = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(patsubst %.cxx,%.o,$(notdir $(SRCS)))))

all: $(PLUGIN)

$(PLUGIN): $(OBJECTS)
ifeq (solaris,$(findstring solaris,$(target_os)))

	$(Q_LD)$(CXX) $(LDSO) $@ -o $@ $^ $(EXTRALIBS) $(LDFLAGS)
else
	$(Q_LD)$(CXX) $(LDSO) -o $@ $^ $(EXTRALIBS) $(LDFLAGS)

endif	

install:
	mkdir -p $(DESTDIR)$(libdir)/$(AC_PLUGIN_DIR)
	install $(PLUGIN) $(DESTDIR)$(libdir)/$(AC_PLUGIN_DIR)

uninstall:
	rm -f $(DESTDIR)$(libdir)/$(AC_PLUGIN_DIR)/$(PLUGIN)

clean:
	rm -f $(OBJECTS) $(PLUGIN)

###########################################

