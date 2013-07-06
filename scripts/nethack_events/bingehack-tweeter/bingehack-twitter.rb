#!/usr/bin/env ruby

require 'rubygems'
require 'bundler/setup'

require 'twitter'
require 'net/ldap'
require 'yaml'

if ARGV.length != 9
  $stderr.puts "Usage: #{$0} name role race gender align deathlev hp maxhp death"
  exit(false)
end

name, role, race, gender, align, deathlev, hp, maxhp, death = *ARGV

$config = YAML.load_file('twitter-config.yml')

def get_twitter_username(name)
  include Net
  config = $config['ldap']
  ldap = LDAP.new(
    host: config['host'],
    port: config['port'],
    encryption: :simple_tls,
    auth: {
      method: :simple,
      username: config['binddn'],
      password: config['bindpw']
    }
  )
  return nil unless ldap.bind
  twitterName = nil
  ldap.search(
    base: 'ou=Users,dc=csh,dc=rit,dc=edu',
    filter: LDAP::Filter.eq('uid', name),
    scope: LDAP::SearchScope_SingleLevel,
    attributes: ['twitterName']
  ) do |entry|
    entry.each do |attribute, value|
      twitterName = value.first
    end
  end
  twitterName
rescue
  nil
end

twitter_username = get_twitter_username(name)
if twitter_username
  twitter_username = "@#{twitter_username}"
else
  twitter_username = name
end
name = twitter_username

Twitter.configure do |config|
  yml_config = $config['twitter']
  config.consumer_key = yml_config['consumer_key']
  config.consumer_secret = yml_config['consumer_secret']
  config.oauth_token = yml_config['oauth_token']
  config.oauth_token_secret = yml_config['oauth_token_secret']
end

client = Twitter::Client.new

client.update("#{name}-#{role}-#{race}-#{gender}-#{align} #{death} on level #{deathlev} (HP: #{hp} [#{maxhp}])"[0,140])
