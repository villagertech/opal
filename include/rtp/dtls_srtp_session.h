/*
 * dtls_srtp_session.h
 *
 * SRTP protocol session handler with DTLS key exchange
 *
 * OPAL Library
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Sysolyatin Pavel
 *
 * Contributor(s): ______________________________________.
 */

#ifndef OPAL_RTP_DTLS_SRTP_SESSION_H
#define OPAL_RTP_DTLS_SRTP_SESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal_config.h>
#include <sdp/ice.h>
#include <rtp/srtp_session.h>

#if OPAL_SRTP
#include <ptclib/pssl.h>
#include <ptclib/pstun.h>
#include <ptclib/qchannel.h>


/**String option key to an integer indicating the time in seconds to
   wait for any DTLS messages. Default 2.
  */
#define OPAL_OPT_DTLS_TIMEOUT "DTLS-Timeout"


#if OPAL_ICE
typedef OpalICEMediaTransport OpalDTLSMediaTransportParent;
#else
typedef OpalUDPMediaTransport OpalDTLSMediaTransportParent;
#endif

class OpalDTLSMediaTransport : public OpalDTLSMediaTransportParent
{
    PCLASSINFO(OpalDTLSMediaTransport, OpalDTLSMediaTransportParent);
  public:
    OpalDTLSMediaTransport(const PString & name, bool passiveMode, const PSSLCertificateFingerprint& fp);

    virtual bool Open(OpalMediaSession & session, PINDEX count, const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool IsEstablished() const;
    virtual bool GetKeyInfo(OpalMediaCryptoKeyInfo * keyInfo[2]);

    void SetPassiveMode(bool passive);
    PSSLCertificateFingerprint GetLocalFingerprint(PSSLCertificateFingerprint::HashType hashType) const;
    bool SetRemoteFingerprint(const PSSLCertificateFingerprint& fp);
    PSSLCertificateFingerprint GetRemoteFingerprint() const;

  protected:
    virtual PChannel * AddWrapperChannels(SubChannels subchannel, PChannel * channel);

    class DTLSChannel : public PSSLChannelDTLS
    {
        PCLASSINFO(DTLSChannel, PSSLChannelDTLS);
      public:
        DTLSChannel(OpalDTLSMediaTransport & transport, PChannel * channel);
        ~DTLSChannel() { Close(); }
        virtual bool Read(void * buf, PINDEX len);
        virtual int BioRead(char * buf, int len);
        virtual int BioWrite(const char * buf, int len);
      protected:
        OpalDTLSMediaTransport & m_transport;
        // Used to cache a ClientHello received before we're ready to handshake.
        // Unfortunately Firefox likes to fragment this over two packets after the
        // first 3 ignored attempts (~350ms).
        std::list<PBYTEArray>    m_lastReceivedPackets;
        // The final handshake response packet (for retransmission in case it is lost)
        PBYTEArray               m_lastResponseData;
        PINDEX                   m_lastResponseLength;
    };
    friend class DTLSChannel;

    bool InternalPerformHandshake(DTLSChannel * channel);
    virtual bool PerformHandshake(DTLSChannel & channel);
    PDECLARE_SSLVerifyNotifier(OpalDTLSMediaTransport, OnVerify);

    atomic<bool>        m_passiveMode;
    PTimeInterval       m_handshakeTimeout;
    unsigned            m_MTU;
    PSSLCertificate     m_certificate;
    PSSLPrivateKey      m_privateKey;
    PSSLCertificateFingerprint m_remoteFingerprint;
    std::auto_ptr<OpalMediaCryptoKeyInfo> m_keyInfo[2];

  friend class OpalDTLSContext;

  P_REMOVE_VIRTUAL(DTLSChannel*,CreateDTLSChannel(),NULL);
  P_REMOVE_VIRTUAL_VOID(PerformHandshake(PChannel*));
};


class OpalDTLSSRTPSession : public OpalSRTPSession
{
    PCLASSINFO(OpalDTLSSRTPSession, OpalSRTPSession);
  public:
    static const PCaselessString & RTP_DTLS_SAVP();
    static const PCaselessString & RTP_DTLS_SAVPF();

    OpalDTLSSRTPSession(const Init & init);
    ~OpalDTLSSRTPSession();

    virtual const PCaselessString & GetSessionType() const { return RTP_DTLS_SAVP(); }

    // New members
    void SetPassiveMode(bool passive);
    bool IsPassiveMode() const;

    PSSLCertificateFingerprint GetLocalFingerprint(PSSLCertificateFingerprint::HashType hashType) const;
    void SetRemoteFingerprint(const PSSLCertificateFingerprint& fp);

  protected:
    virtual OpalMediaTransport * CreateMediaTransport(const PString & name);

    bool                       m_passiveMode;
    PSSLCertificateFingerprint m_earlyRemoteFingerprint;
};


#endif // OPAL_SRTP

#endif // OPAL_RTP_DTLS_SRTP_SESSION_H
