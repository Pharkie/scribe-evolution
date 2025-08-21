/**
 * Scribe Evolution Printer - Pipedream MQTT Bridge
 *
 * A Pipedream component for sending messages to Scribe Evolution thermal printers via MQTT.
 * This component receives HTTP requests and forwards them as MQTT messages to
 * specified remote printers.
 * 
 * Author: Adam Knowles
 * License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 * 
 * You are free to:
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 * 
 * Under the following terms:
 * - Attribution — You must give appropriate credit, provide a link to the license,
 *   and indicate if changes were made.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material, you must
 *   distribute your contributions under the same license as the original.
 * 
 * For full license text, see: https://creativecommons.org/licenses/by-nc-sa/4.0/
 */

import mqtt from "mqtt";

function mqttPublishAsync(client, topic, payload) {
  return new Promise((resolve, reject) => {
    client.publish(topic, payload, { qos: 1 }, (err) => {
      if (err) reject(err);
      else resolve();
    });
  });
}

function mqttConnectAsync(opts) {
  return new Promise((resolve, reject) => {
    const client = mqtt.connect(opts);

    client.once("connect", () => resolve(client));
    client.once("error", (err) => reject(err));
  });
}

export default defineComponent({
  props: {},
  async run({ steps, $ }) {
    let host = process.env.MQTT_host;
    const port = process.env.MQTT_port;
    const username = process.env.MQTT_username;
    const password = process.env.MQTT_password;

    if (!host || !port || !username || !password) {
      const missingEnvVars = [];
      if (!host) missingEnvVars.push("MQTT_host");
      if (!port) missingEnvVars.push("MQTT_port");
      if (!username) missingEnvVars.push("MQTT_username");
      if (!password) missingEnvVars.push("MQTT_password");
      
      await $.respond({
        status: 500,
        body: {
          error: `Missing required environment variables: ${missingEnvVars.join(", ")}`,
        },
      });
      return;
    }

    if (host.includes(":")) {
      host = host.split(":")[0];
    }

    // Extract data from HTTP POST body (steps.trigger.event is already an object)
    const data = steps.trigger.event || {};
    
    const remote_printer = data.remote_printer;
    const message = data.message;
    const timestamp = data.timestamp;
    const sender = data.sender;

    // Validate required fields
    const missingFields = [];
    if (!remote_printer) missingFields.push("remote_printer");
    if (!message) missingFields.push("message");
    if (!timestamp) missingFields.push("timestamp");
    if (!sender) missingFields.push("sender");

    if (missingFields.length > 0) {
      await $.respond({
        status: 400,
        body: { error: `Missing required fields: ${missingFields.join(", ")}` },
      });
      return;
    }

    const topic = `scribe/${remote_printer}/inbox`;
    const payload = JSON.stringify({ message, timestamp, sender });

    try {
      const client = await mqttConnectAsync({
        host,
        port: Number(port),
        protocol: "mqtts",
        username,
        password,
        rejectUnauthorized: false,
      });

      await mqttPublishAsync(client, topic, payload);
      console.log(`MQTT message published with broker confirmation to topic: ${topic} (${payload.length} bytes)`);

      await new Promise((resolve) => {
        client.end(true, () => {
          client.removeAllListeners();
          resolve();
        });
      });

      await $.respond({
        status: 200,
        body: { status: "Published", topic, payload },
      });
    } catch (err) {
      await $.respond({
        status: 500,
        body: { error: `MQTT Error: ${err.message}` },
      });
    }
  },
});