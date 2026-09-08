import type { Config } from '@netlify/functions'

export default async () => Response.json({ status: 'ok', service: 'Simply Compiler Backend' })

export const config: Config = {
  path: '/health',
}
