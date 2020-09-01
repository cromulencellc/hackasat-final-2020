begin
    # try loading local dependencies first (bundle install --standalone)
    require "./bundle/bundler/setup"
    require "bundler"
    Bundler.require
rescue LoadError
    # fall back to system-wide dependencies (bundle install or other)
    require "bundler"
    Bundler.setup
    Bundler.require
end

# set up logging
#log = File.new("logs/sinatra.log", "a")
#STDERR.reopen(log)

# run our application's controller
require "./controller"
run LaunchSIP::Controller
