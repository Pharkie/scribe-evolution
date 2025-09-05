# MQTT Security Model

## Current Implementation (Trust-Based)

The Scribe printer system uses a trust-based MQTT security model suitable for home and controlled network environments.

### Message Format

MQTT messages use structured JSON format:

```json
{
  "header": "JOKE",
  "body": "Why did the chicken cross the road? To get to the other side!",
  "sender": "Pharkie"
}
```

### Security Characteristics

**Sender Identity:**

- Self-reported in the `sender` field
- No cryptographic verification
- Trust-based model assumes good actors

**Message Authentication:**

- No message signing or verification
- No protection against message tampering
- Relies on MQTT broker security and TLS encryption

**Network Security:**

- TLS encryption for MQTT connections
- CA certificate verification for broker authenticity
- Protects against network-level eavesdropping

## Security Limitations

⚠️ **Important Security Warnings:**

1. **Sender Spoofing**: Any MQTT client can claim any identity
2. **Message Forgery**: No protection against fake messages
3. **Trust Required**: All network participants must be trusted
4. **No Authorization**: No permission system for who can send what

## Suitable Environments

✅ **Appropriate for:**

- Home networks with trusted devices
- Private networks with known participants
- Development and testing environments
- Controlled corporate networks

❌ **NOT suitable for:**

- Public networks or untrusted environments
- Systems requiring sender authentication
- High-security applications
- Multi-tenant environments

## Future Security Roadmap

### Phase 1: Current (Trust-Based) ✅

- Structured message format
- TLS encryption
- Self-reported identity
- **Status**: Implemented

### Phase 2: Optional Message Signing

- Add optional `signature` field to messages
- RSA/ECDSA signatures for sender verification
- Backward compatible with unsigned messages
- **Status**: Planned

### Phase 3: Public Key Infrastructure

- Device key generation and storage
- Public key distribution system
- Signature verification on receive
- **Status**: Future consideration

### Phase 4: Broker-Level Access Control

- MQTT broker ACL configuration
- Topic-based permissions per device
- Fine-grained send/receive controls
- **Status**: Future consideration

## Risk Assessment

### Low Risk Scenarios

- Home use with family members
- Personal development projects
- Controlled office environments

### Medium Risk Scenarios

- Shared networks with unknown devices
- IoT networks with multiple vendors
- Remote access scenarios

### High Risk Scenarios

- Public or untrusted networks
- Commercial or industrial use
- Systems handling sensitive data

## Mitigation Strategies

For enhanced security in higher-risk environments:

1. **Network Isolation**: Use dedicated VLAN or subnet
2. **Broker Security**: Configure MQTT broker with authentication
3. **Monitoring**: Log all MQTT messages for audit trails
4. **Device Certificates**: Use client certificates for broker authentication
5. **Message Validation**: Implement additional content validation

## Implementation Notes

- All MQTT messages MUST use structured format (header + body + sender)
- Legacy message formats are rejected (fail-fast principle)
- Receiving printers construct final headers as: `"{header} from {sender}"`
- Empty sender fields result in headers without "from" suffix

## Upgrading Security

When implementing Phase 2 (message signing):

1. Add optional `signature` field to message format
2. Generate/store device private keys securely
3. Implement signature verification on receive
4. Maintain backward compatibility with unsigned messages
5. Add configuration option to require signatures

## Contact and Support

For security-related questions or to report vulnerabilities:

- Create issue in project repository
- Follow responsible disclosure practices
- Security fixes will be prioritized
