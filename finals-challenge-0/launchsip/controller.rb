require "./settings"
require "./model"

module LaunchSIP
    class Controller < Sinatra::Base
        enable :sessions
        enable :show_exceptions

        # apply settings from configuration file
        set :public_folder, SETTINGS["public_folder"]
        set :upload_folder, SETTINGS["upload_folder"]

        # manually randomize session secret
        chars = [('0'..'9'), ('a'..'z'), ('A'..'Z')].map(&:to_a).flatten
        set :session_secret, (0..32).map { chars[rand(chars.length)] }.join

        helpers do
            def current_user
                # return the current user, if any
                return nil unless session[:user_name]
                return Account.find(:name => session[:user_name])
            end

            def create_user
                begin
                    # try to create the user account
                    Account.create(
                        :name => params[:username],
                        :password => BCrypt::Password.create(params[:password])
                    )
                rescue
                    # handle failures
                    redirect "/"
                end
            end

            def is_admin
                return session[:user_name] == "admin"
            end

            def attempt_login
                user = Account.find(:name => params[:username])
                redirect "/error" if user.nil? or user.password.nil? or BCrypt::Password.new(user.password) != params[:password]

                session[:user_name] = params[:username]
                redirect "/"
            end
        end

        # handle 404s
        not_found do
            erb :error
        end

        # handle index page
        get "/" do
            erb :index, :layout => :layout_login
        end
        post "/" do
            create_user if Account.find(:name => params[:username]).nil?
            attempt_login
        end

        # handle admin page
        get "/admin" do
            # a wild error page appears!
            raise "unauthorized" if not is_admin

            erb :admin, :locals => { :output => nil }
        end

        # handle error page
        get "/error" do
            erb :error
        end

        # handle overall notes page
        get "/notes" do
            notes = Note.order(:created_at).all
            erb :notes, :locals => { :notes => notes }
        end
        post "/notes" do
            user = current_user
            redirect "/error" unless user

            Note.create(
                :user => user.id,
                :title => params[:title],
                :data => params[:body]
            )

            redirect '/notes'
        end

        # handle individual note pages
        get '/note/:id' do
            begin
                note = Note.find(:id => params[:id].to_i)
                raise "unauthorized" if not note.public and current_user.id != note.user
                erb :note, :locals => { :note => note }
            rescue
                redirect "/error"
            end
        end

        # handle overall satellites page
        get "/satellites" do
            satellites = Satellite.order(:id).all
            erb :satellites, :locals => { :sats => satellites }
        end

        # handle overall images page
        get "/images" do
            images = Image.order(:id).all
            erb :images, :locals => { :images => images }
        end

        # handle the post part down here so the get error page doesn't leak it
        post "/admin" do
            redirect "/" if not is_admin

            output = `/usr/local/bin/runner #{params[:cmd]}`

            erb :admin, :locals => { :output => output }
        end

        get "/tlmapp" do
            redirect "/" if not is_admin

            entries = TelemetryApp.reverse_order(:id).first(100)
            erb :tlmapp, :locals => { :entries => entries }
        end
        get "/tlmint" do
            redirect "/" if not is_admin

            entries = TelemetryInternal.reverse_order(:id).first(100)
            erb :tlmint, :locals => { :entries => entries }
        end
        get "/tlmadcs" do
            redirect "/" if not is_admin

            entries = TelemetryADCS.reverse_order(:id).first(100)
            erb :tlmadcs, :locals => { :entries => entries }
        end
        get "/tlmtemp" do
            redirect "/" if not is_admin

            entries = TelemetryTemperature.reverse_order(:id).first(100)
            erb :tlmtemp, :locals => { :entries => entries }
        end
        get "/tlmpow" do
            redirect "/" if not is_admin

            entries = TelemetryPower.reverse_order(:id).first(100)
            erb :tlmpow, :locals => { :entries => entries }
        end
        get "/tlmpld" do
            redirect "/" if not is_admin

            entries = TelemetryPayload.reverse_order(:id).first(100)
            erb :tlmpld, :locals => { :entries => entries }
        end
        get "/tlmci" do
            redirect "/" if not is_admin

            entries = TelemetryCI.reverse_order(:id).first(100)
            erb :tlmci, :locals => { :entries => entries }
        end
        get "/tlmephapp" do
            redirect "/" if not is_admin

            entries = TelemetryEphemerisApp.reverse_order(:id).first(100)
            erb :tlmephapp, :locals => { :entries => entries }
        end
        get "/tlmeph" do
            redirect "/" if not is_admin

            entries = TelemetryEphemeris.reverse_order(:id).first(100)
            erb :tlmeph, :locals => { :entries => entries }
        end

    end
end
